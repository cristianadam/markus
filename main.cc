#include <iostream>
#include <sstream>
#include <string>

#include "markus.h"

namespace {

void PrintUsage(const char* program_name) {
  std::cerr << "Usage: " << program_name << " [--ast]\n";
  std::cerr << "\n";
  std::cerr << "Reads markdown from stdin and outputs HTML to stdout.\n";
  std::cerr << "\n";
  std::cerr << "Options:\n";
  std::cerr << "  --ast    Print the AST instead of HTML output\n";
  std::cerr << "  --help   Show this help message\n";
}

std::string ReadStdin() {
  std::stringstream buffer;
  buffer << std::cin.rdbuf();
  return buffer.str();
}

}  // namespace

int main(int argc, char* argv[]) {
  bool ast_mode = false;

  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    if (arg == "--help" || arg == "-h") {
      PrintUsage(argv[0]);
      return 0;
    }
    if (arg == "--ast") {
      ast_mode = true;
    }
  }

  std::string markdown = ReadStdin();

  if (ast_mode) {
    markus::Document doc = markus::Parse(markdown);
    std::cout << markus::DebugAst(doc);
  } else {
    std::cout << markus::MarkdownToHtml(markdown);
  }

  return 0;
}
