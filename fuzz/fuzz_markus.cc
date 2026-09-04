// libFuzzer / AFL harness for the markus.h Markdown parser and renderer.
//
// Layout mirrors cmark-gfm's fuzz/ harness: a fixed-width "config" prefix is
// prepended to the raw Markdown body. The config selects which GFM extensions
// are enabled and drives a quadratic amplifier (a repeated segment) so the
// fuzzer can reach the parser's worst-case paths. Every input is run through
// both the batch path (Parse + RenderHtml + AST debug) and the streaming path
// (StreamingMarkdownParser), in "whole document" and "many small chunks"
// shapes.
//
// Build (libFuzzer; needs a clang that ships the fuzzer runtime, not the
// default Apple clang):
//   clang++ -std=c++20 -fsanitize=fuzzer,address -g fuzz_markus.cc -o fuzz_markus
//
// Build (standalone / AFL driver; any compiler):
//   clang++ -std=c++20 -fsanitize=address,undefined -g \
//           -DMARKUS_FUZZ_STANDALONE fuzz_markus.cc -o fuzz_markus_standalone
//
// Replay (standalone):
//   ./fuzz_markus_standalone file.md
//   ./fuzz_markus_standalone corpus_dir/     # replay every file in the dir

#include "markus.h"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

namespace {

// Fixed-width fuzzer config prepended to the Markdown body.
struct FuzzConfig {
  uint32_t extensions;   // bitmask, see kExt* below
  uint32_t splitpoint;   // offset in the body where the repeat segment begins
  uint32_t repeatlen;    // length of the repeated segment (0 = no amplification)
  uint32_t stream_chunk; // streaming feed chunk size (0 = auto / whole document)
};

constexpr uint32_t kExtTables = 1u << 0;
constexpr uint32_t kExtAutolink = 1u << 1;
constexpr uint32_t kExtStrikethrough = 1u << 2;
constexpr uint32_t kExtTasklist = 1u << 3;
constexpr uint32_t kExtTagfilter = 1u << 4;

// Bound the amplified input so a hostile config cannot OOM the harness, while
// staying large enough to trigger deep-nesting (stack) and quadratic-time
// bugs. markus has O(n^2) paths on adversarial input, so this cap also keeps
// per-unit worst-case cost bounded; raise it to probe deeper-nesting/very
// large inputs if the fuzzer's -timeout is available (CI / Linux).
constexpr size_t kMaxAmplified = 1u << 16;  // 64 KiB

// The streaming parser re-parses the settled buffer on every Feed (O(n) per
// call, O(n * feeds) total). Bound both the streamed input size and the number
// of feeds so a hostile config (e.g. stream_chunk = 1) cannot turn one unit
// into O(n^2) work.
constexpr size_t kMaxStreamed = 1u << 15;   // 32 KiB
constexpr size_t kMaxStreamFeeds = 64;

markus::Options ToOptions(uint32_t ext) {
  markus::Options o;
  o.enable_tables = (ext & kExtTables) != 0;
  o.enable_autolink = (ext & kExtAutolink) != 0;
  o.enable_strikethrough = (ext & kExtStrikethrough) != 0;
  o.enable_tasklist = (ext & kExtTasklist) != 0;
  o.enable_tagfilter = (ext & kExtTagfilter) != 0;
  return o;
}

// Quadratic amplifier: emit body[0,split), then repeat
// body[split,split+repeat) until the buffer is (near) full, then append the
// remaining tail. Mirrors cmark-gfm's fuzz_quadratic.c so the fuzzer reaches
// the same worst-case parsing shapes (deeply nested/long repeated structures).
std::string Amplify(const char* body, size_t size, uint32_t splitpoint,
                    uint32_t repeatlen) {
  if (splitpoint > size || repeatlen > size - splitpoint || repeatlen == 0) {
    return std::string(body, size);  // invalid / no-op config: use verbatim
  }
  std::string out;
  out.reserve(kMaxAmplified);
  out.append(body, splitpoint);
  const size_t tail_start = splitpoint + repeatlen;
  const size_t tail_len = size - tail_start;
  while (out.size() + repeatlen + tail_len <= kMaxAmplified) {
    out.append(body + splitpoint, repeatlen);
  }
  if (out.size() + tail_len <= kMaxAmplified) {
    out.append(body + tail_start, tail_len);
  }
  return out;
}

void RunBatch(const std::string& md, const markus::Options& o) {
  // Parse + render: the primary quadratic target.
  markus::Document doc = markus::Parse(md, o);
  std::pmr::string html = markus::RenderHtml(doc, o);
  (void)html;
  // Exercise the AST debug printer (a second, different node traversal).
  std::pmr::string ast = markus::DebugAst(doc);
  (void)ast;
}

void RunStreaming(const std::string& md, const markus::Options& o, uint32_t chunk) {
  markus::StreamingMarkdownParser parser(o);
  std::string collected;
  parser.setOutputCallback([&collected](std::string_view h) {
    collected.append(h.data(), h.size());
  });
  if (chunk == 0) {
    parser.Feed(md);
  } else {
    size_t c = chunk;
    if (c < 1) c = 1;
    for (size_t i = 0; i < md.size(); i += c) {
      const size_t n = (md.size() - i < c) ? (md.size() - i) : c;
      parser.Feed(md.substr(i, n));
    }
  }
  parser.Flush();
  (void)collected;
}

}  // namespace

extern "C" int LLVMFuzzerInitialize(int* /*argc*/, char*** /*argv*/) { return 0; }

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  if (size < sizeof(FuzzConfig)) return 0;
  FuzzConfig cfg;
  memcpy(&cfg, data, sizeof(cfg));

  const char* body = reinterpret_cast<const char*>(data + sizeof(FuzzConfig));
  const size_t body_size = size - sizeof(FuzzConfig);
  if (body_size == 0) return 0;

  const markus::Options opts = ToOptions(cfg.extensions);
  const std::string md =
      Amplify(body, body_size, cfg.splitpoint, cfg.repeatlen);

  RunBatch(md, opts);

  // Streaming: cover "whole document at once" plus a chunked run, both on a
  // size-bounded prefix with a bounded feed count, so a hostile stream_chunk
  // cannot turn a single unit into O(n^2) work.
  const size_t stream_n = (md.size() < kMaxStreamed) ? md.size() : kMaxStreamed;
  const std::string smd(md.data(), stream_n);
  RunStreaming(smd, opts, /*chunk=*/0);
  const size_t min_chunk = (stream_n + kMaxStreamFeeds - 1) / kMaxStreamFeeds;
  size_t sc = cfg.stream_chunk;
  if (sc < min_chunk) sc = min_chunk;
  RunStreaming(smd, opts, static_cast<uint32_t>(sc));
  return 0;
}

#ifdef MARKUS_FUZZ_STANDALONE
#include <dirent.h>
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <vector>

// Standalone driver: provides its own main so the harness runs without a
// libFuzzer runtime. Usable for manual replay or as an AFL++ target (pass the
// input path as argv[1], e.g. `afl-fuzz ... ./fuzz_markus_standalone @@`).
namespace {
int RunFile(const char* path) {
  std::ifstream in(path, std::ios::binary);
  if (!in) {
    std::fprintf(stderr, "cannot open %s\n", path);
    return 2;
  }
  std::ostringstream ss;
  ss << in.rdbuf();
  const std::string buf = ss.str();
  const uint8_t* p = reinterpret_cast<const uint8_t*>(buf.data());
  LLVMFuzzerTestOneInput(p, buf.size());
  return 0;
}
}  // namespace

int main(int argc, char* argv[]) {
  if (LLVMFuzzerInitialize(&argc, &argv) != 0) return 1;
  if (argc < 2) {
    std::fprintf(stderr,
                 "usage: %s <file | corpus_dir>\n"
                 "  <file>       replay a single input\n"
                 "  <corpus_dir> replay every regular file in the directory\n",
                 argv[0]);
    return 2;
  }
  const std::string arg = argv[1];
  struct stat st;
  if (stat(arg.c_str(), &st) == 0 && S_ISDIR(st.st_mode)) {
    DIR* d = opendir(arg.c_str());
    if (!d) return 2;
    int rc = 0;
    struct dirent* e;
    while ((e = readdir(d)) != nullptr) {
      if (e->d_name[0] == '.') continue;
      const std::string path = arg + "/" + e->d_name;
      struct stat fs;
      if (stat(path.c_str(), &fs) != 0 || !S_ISREG(fs.st_mode)) continue;
      rc |= RunFile(path.c_str());
    }
    closedir(d);
    return rc;
  }
  return RunFile(arg.c_str());
}
#endif  // MARKUS_FUZZ_STANDALONE
