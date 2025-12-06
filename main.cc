// Copyright 2024 Hyde Authors
// SPDX-License-Identifier: MIT

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "hyde.h"

namespace {

void PrintUsage(const char* program_name) {
  std::cerr << "Usage: " << program_name << " <input.md> <output.html>\n";
  std::cerr << "       " << program_name << " --ast <input.md>\n";
  std::cerr << "\n";
  std::cerr << "Options:\n";
  std::cerr << "  --ast    Print the AST instead of HTML output\n";
  std::cerr << "  --help   Show this help message\n";
}

std::string ReadFile(const std::string& path) {
  std::ifstream file(path);
  if (!file) {
    std::cerr << "Error: Cannot open file: " << path << "\n";
    std::exit(1);
  }
  std::stringstream buffer;
  buffer << file.rdbuf();
  return buffer.str();
}

void WriteFile(const std::string& path, const std::string& content) {
  std::ofstream file(path);
  if (!file) {
    std::cerr << "Error: Cannot write to file: " << path << "\n";
    std::exit(1);
  }
  file << content;
}

}  // namespace

int main(int argc, char* argv[]) {
  if (argc < 2) {
    PrintUsage(argv[0]);
    return 1;
  }

  std::string first_arg = argv[1];

  if (first_arg == "--help" || first_arg == "-h") {
    PrintUsage(argv[0]);
    return 0;
  }

  if (first_arg == "--ast") {
    if (argc < 3) {
      std::cerr << "Error: Missing input file for --ast\n";
      PrintUsage(argv[0]);
      return 1;
    }
    std::string input_path = argv[2];
    std::string markdown = ReadFile(input_path);
    hyde::Document doc = hyde::Parse(markdown);
    std::cout << hyde::DebugAst(doc);
    return 0;
  }

  if (argc < 3) {
    std::cerr << "Error: Missing output file\n";
    PrintUsage(argv[0]);
    return 1;
  }

  std::string input_path = argv[1];
  std::string output_path = argv[2];

  std::string markdown = ReadFile(input_path);
  std::string html = hyde::MarkdownToHtml(markdown);
  WriteFile(output_path, html);

  std::cerr << "Converted " << input_path << " -> " << output_path << "\n";
  return 0;
}
