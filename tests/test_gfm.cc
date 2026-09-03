#include <cstdio>
#include <cstdlib>
#include <sstream>
#include <string>

#ifdef _WIN32
#include <windows.h>
#define popen _popen
#define pclose _pclose
#else
#include <sys/wait.h>
#endif

#include "gtest/gtest.h"
#include "markus.h"

namespace {

std::string GetMainBinaryPath() {
  const char* path = std::getenv("MARKUS_MAIN");
  if (path && path[0] != '\0') {
    return path;
  }
  return "./build/main";
}

std::string GetGfmSpecPath() {
  const char* path = std::getenv("MARKUS_GFM_SPEC");
  if (path && path[0] != '\0') {
    return path;
  }
  return "cmark-gfm/test/spec.txt";
}

// Runs the cmark-gfm GFM spec examples whose section matches `pattern` against
// markus's main binary. The runner (cmark-gfm/test/spec_tests.py) drives the
// program in external mode, appending `--unsafe` and a `-e <ext>` flag per
// required extension; markus currently ignores these and renders plain
// CommonMark, so these tests fail until each GFM feature is implemented.
std::string RunGfmSection(const std::string& pattern,
                          const std::string& main_path,
                          const std::string& spec_path) {
  std::ostringstream cmd;
  cmd << "python3 cmark-gfm/test/spec_tests.py"
      << " --program " << main_path << " -s " << spec_path << " -P \""
      << pattern << "\""
      << " 2>&1";

  FILE* pipe = popen(cmd.str().c_str(), "r");
  if (!pipe) return "ERROR: Failed to start python3";

  std::string output;
  char buffer[256];
  while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
    std::string line(buffer);
    if (line.find(" passed, ") != std::string::npos &&
        line.find(" failed, ") != std::string::npos &&
        line.find(" skipped") != std::string::npos) {
      continue;
    }
    output += line;
  }

  int exit_code;
#ifdef _WIN32
  exit_code = pclose(pipe);
#else
  int status = pclose(pipe);
  if (WIFEXITED(status)) {
    exit_code = WEXITSTATUS(status);
  } else {
    return "ERROR: pclose failed";
  }
#endif

  if (exit_code == 0) {
    return "PASS";
  }
  std::ostringstream err;
  err << "EXIT_CODE=" << exit_code << "\n" << output;
  return err.str();
}

}  // namespace

// GFM extension features tracked against cmark-gfm's GFM spec, so that as
// markus implements each one, the corresponding test goes green. Opt-in via
// -DMARKUS_BUILD_GFM_TESTS=ON (excluded from the default CommonMark run).
//
// Task list items are intentionally omitted: their GFM spec examples are marked
// `disabled` (checkbox markup is left to implementors), so they are not
// spec-testable.
#define RUN_GFM_SECTION(test_name, pattern)                                    \
  TEST(GFM, test_name) {                                                       \
    const std::string main_path = GetMainBinaryPath();                         \
    const std::string spec_path = GetGfmSpecPath();                            \
    std::string result = RunGfmSection(pattern, main_path, spec_path);         \
    EXPECT_EQ("PASS", result) << "GFM section \"" << pattern << "\" failed:\n" \
                              << result;                                       \
  }

RUN_GFM_SECTION(Tables, "Tables")
RUN_GFM_SECTION(Strikethrough, "Strikethrough")
RUN_GFM_SECTION(AutolinkExtensions, "Autolinks \\(extension\\)")
RUN_GFM_SECTION(DisallowedRawHtml, "Disallowed")
