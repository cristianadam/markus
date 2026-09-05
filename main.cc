#include <iostream>
#include <sstream>
#include <string>

#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#endif

#include "markus.h"

namespace {

void PrintUsage(const char* program_name) {
  std::cerr << "Usage: " << program_name
            << " [--ast] [--stream] [--unsafe] [-e <ext> ...]\n";
  std::cerr << "\n";
  std::cerr << "Reads markdown from stdin and outputs HTML to stdout.\n";
  std::cerr << "\n";
  std::cerr << "Options:\n";
  std::cerr << "  --ast      Print the AST instead of HTML output\n";
  std::cerr << "  --stream   Stream HTML output as blocks are parsed "
                "(progressive rendering)\n";
  std::cerr << "  --unsafe   Accept (and render) raw HTML (accepted for cmark "
                "compatibility)\n";
  std::cerr << "  -e <ext>   Enable a GFM extension (table, autolink, "
                "strikethrough, tasklist, tagfilter)\n";
  std::cerr << "  --help     Show this help message\n";
}

std::string ReadStdin() {
  std::stringstream buffer;
  buffer << std::cin.rdbuf();
  return buffer.str();
}

}  // namespace

int main(int argc, char* argv[]) {
#ifdef _WIN32
  _setmode(_fileno(stdout), _O_BINARY);
  _setmode(_fileno(stdin), _O_BINARY);
#endif

  bool ast_mode = false;
  bool stream_mode = false;
  markus::Options options;

  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    if (arg == "--help" || arg == "-h") {
      PrintUsage(argv[0]);
      return 0;
    }
    if (arg == "--ast") {
      ast_mode = true;
    } else if (arg == "--stream") {
      stream_mode = true;
    } else if (arg == "--unsafe") {
      // Raw HTML is always passed through; the flag is accepted for
      // cmark/cmark-gfm compatibility.
    } else if (arg == "-e") {
      if (i + 1 < argc) {
        std::string ext = argv[i + 1];
        if (ext == "table") {
          options.enable_tables = true;
        } else if (ext == "autolink") {
          options.enable_autolink = true;
        } else if (ext == "strikethrough") {
          options.enable_strikethrough = true;
        } else if (ext == "tasklist") {
          options.enable_tasklist = true;
        } else if (ext == "tagfilter") {
          options.enable_tagfilter = true;
        }
      }
      ++i;  // Consume the extension name.
    }
  }

  if (stream_mode) {
    // Read stdin incrementally and emit HTML as blocks finalize, so the flag
    // exercises the progressive path even for piped input.
    markus::StreamingMarkdownParser parser(options);
    parser.setOutputCallback([](std::string_view html) {
      std::cout.write(html.data(), html.size());
      std::cout.flush();
    });
    char buf[256];
    try {
      while (std::cin.read(buf, sizeof(buf)), std::cin.gcount() > 0) {
        parser.Feed(std::string_view(buf, std::cin.gcount()));
      }
      parser.Flush();
    } catch (const std::length_error& e) {
      std::cerr << "markus: " << e.what() << "\n";
      return 1;
    }
    return 0;
  }

  std::string markdown = ReadStdin();

  if (ast_mode) {
    markus::Document doc = markus::Parse(markdown, options);
    std::cout << markus::DebugAst(doc);
  } else {
    std::cout << markus::MarkdownToHtml(markdown, options);
  }

  return 0;
}
