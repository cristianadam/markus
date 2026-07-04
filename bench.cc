#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dirent.h>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "markus.h"

// cmark headers
#include "commonmark-spec/src/cmark.h"

namespace {

std::string ReadFile(const std::string& path) {
  std::ifstream ifs(path);
  return std::string((std::istreambuf_iterator<char>(ifs)),
                     (std::istreambuf_iterator<char>()));
}

double TimeMarkus(const std::string& input, int iterations) {
  auto start = std::chrono::high_resolution_clock::now();
  for (int i = 0; i < iterations; ++i) {
    std::pmr::string result = markus::MarkdownToHtml(input);
    if (result.empty() && !input.empty()) {
      std::cerr << "markus produced empty output\n";
    }
  }
  auto end = std::chrono::high_resolution_clock::now();
  return std::chrono::duration<double, std::milli>(end - start).count();
}

double TimeCmark(const std::string& input, int iterations) {
  auto start = std::chrono::high_resolution_clock::now();
  for (int i = 0; i < iterations; ++i) {
    cmark_node* doc = cmark_parse_document(input.c_str(), input.size(),
                                           CMARK_OPT_DEFAULT);
    char* html = cmark_render_html(doc, CMARK_OPT_DEFAULT);
    if (html == nullptr || std::strlen(html) == 0) {
      std::cerr << "cmark produced empty output\n";
    }
    free(html);
    cmark_node_free(doc);
  }
  auto end = std::chrono::high_resolution_clock::now();
  return std::chrono::duration<double, std::milli>(end - start).count();
}

double CalcMean(const std::vector<double>& data) {
  if (data.empty()) return 0.0;
  double sum = 0.0;
  for (auto v : data) sum += v;
  return sum / data.size();
}

double CalcMedian(std::vector<double> data) {
  if (data.empty()) return 0.0;
  std::sort(data.begin(), data.end());
  size_t n = data.size();
  if (n % 2 == 1) return data[n / 2];
  return (data[n / 2 - 1] + data[n / 2]) / 2.0;
}

double CalcStdev(const std::vector<double>& data, double mean_val) {
  if (data.size() < 2) return 0.0;
  double ss = 0.0;
  for (auto v : data) ss += (v - mean_val) * (v - mean_val);
  return std::sqrt(ss / (data.size() - 1));
}

struct BenchResult {
  std::string name;
  size_t bytes;
  double markus_ms;
  double cmark_ms;
  double ratio;
};

std::vector<std::string> GetSampleFiles(const std::string& dir) {
  std::vector<std::string> files;
  DIR* d = opendir(dir.c_str());
  if (!d) return files;
  struct dirent* entry;
  while ((entry = readdir(d)) != nullptr) {
    std::string name = entry->d_name;
    if (name.size() > 2 && name.substr(name.size() - 3) == ".md") {
      files.push_back(dir + "/" + name);
    }
  }
  closedir(d);
  std::sort(files.begin(), files.end());
  return files;
}

std::vector<std::pair<std::string, std::string>> CollectInputs(
    const std::string& samples_dir, const std::string& input_file) {
  std::vector<std::pair<std::string, std::string>> inputs;

  if (!input_file.empty()) {
    inputs.push_back({input_file, ReadFile(input_file)});
  } else {
    auto files = GetSampleFiles(samples_dir);
    for (const auto& f : files) {
      inputs.push_back({f, ReadFile(f)});
    }
  }

  return inputs;
}

void PrintStats(const std::string& label, const std::vector<double>& values) {
  double m = CalcMean(values);
  double med = CalcMedian(values);
  double s = CalcStdev(values, m);
  printf("  %-14s mean = %.4f, median = %.4f, stdev = %.4f\n", label.c_str(), m, med, s);
}

}  // namespace

int main(int argc, char* argv[]) {
  std::string samples_dir = "commonmark-spec/bench/samples";
  std::string input_file;
  int iterations = 100;
  int num_rounds = 5;
  bool iterations_set = false;

  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    if (arg == "--help" || arg == "-h") {
      std::cerr << "Usage: bench [--iterations N] [--input FILE] [--rounds N] [SAMPLES_DIR]\n";
      std::cerr << "\nBenchmarks markus vs cmark on markdown samples.\n";
      std::cerr << "\nOptions:\n";
      std::cerr << "  --iterations N   Number of iterations per sample (default: 100)\n";
      std::cerr << "  --rounds N       Number of benchmark rounds for statistics (default: 5)\n";
      std::cerr << "  --input FILE     Benchmark a single file instead of samples dir\n";
      std::cerr << "  SAMPLES_DIR      Directory with .md files (default: commonmark-spec/bench/samples)\n";
      return 0;
    } else if (arg == "--iterations" || arg == "-n") {
      ++i;
      if (i < argc) { iterations = std::atoi(argv[i]); iterations_set = true; }
    } else if (arg == "--rounds" || arg == "-r") {
      ++i;
      if (i < argc) num_rounds = std::atoi(argv[i]);
    } else if (arg == "--input" || arg == "-f") {
      ++i;
      if (i < argc) input_file = argv[i];
    } else {
      samples_dir = arg;
    }
  }

  auto inputs = CollectInputs(samples_dir, input_file);

  if (inputs.empty()) {
    std::cerr << "No input files found\n";
    return 1;
  }

  // Auto-scale iterations for small files: aim for ~50ms per run minimum
  if (!iterations_set) {
    for (const auto& [name, content] : inputs) {
      if (content.size() < 1024) {
        iterations = std::max(iterations, 500);
      }
    }
  }

  std::vector<BenchResult> results;

  // Print header
  printf("%-30s %8s  %10s  %10s  %8s\n", "Sample", "Bytes", "Markus ms",
         "Cmark ms", "Ratio");
  printf("%-30s %8s  %10s  %10s  %8s\n", "------------------------------",
         "--------", "----------", "----------", "--------");

  double total_markus = 0;
  double total_cmark = 0;
  size_t total_bytes = 0;

  for (const auto& [name, content] : inputs) {
    std::string short_name = name;
    auto slash = short_name.rfind('/');
    if (slash != std::string::npos) {
      short_name = short_name.substr(slash + 1);
    }

    // Warm-up run
    markus::MarkdownToHtml(content);
    {
      cmark_node* doc = cmark_parse_document(content.c_str(), content.size(),
                                             CMARK_OPT_DEFAULT);
      char* html = cmark_render_html(doc, CMARK_OPT_DEFAULT);
      free(html);
      cmark_node_free(doc);
    }

    // Run multiple rounds for statistics
    std::vector<double> markus_times;
    std::vector<double> cmark_times;
    markus_times.reserve(num_rounds);
    cmark_times.reserve(num_rounds);

    for (int round = 0; round < num_rounds; ++round) {
      markus_times.push_back(TimeMarkus(content, iterations));
      cmark_times.push_back(TimeCmark(content, iterations));
    }

    double markus_ms = CalcMean(markus_times);
    double cmark_ms = CalcMean(cmark_times);

    BenchResult r;
    r.name = short_name;
    r.bytes = content.size();
    r.markus_ms = markus_ms;
    r.cmark_ms = cmark_ms;
    r.ratio = cmark_ms > 0 ? markus_ms / cmark_ms : 0;

    printf("%-30s %8zu  %10.2f  %10.2f  %7.2fx\n", r.name.c_str(), r.bytes,
           r.markus_ms, r.cmark_ms, r.ratio);

    PrintStats("markus", markus_times);
    PrintStats("cmark", cmark_times);
    printf("\n");

    total_markus += r.markus_ms;
    total_cmark += r.cmark_ms;
    total_bytes += r.bytes;

    results.push_back(std::move(r));
  }

  printf("%-30s %8s  %10s  %10s  %8s\n", "------------------------------",
         "--------", "----------", "----------", "--------");
  double total_ratio = total_cmark > 0 ? total_markus / total_cmark : 0;
  printf("%-30s %8zu  %10.2f  %10.2f  %7.2fx\n", "TOTAL", total_bytes,
         total_markus, total_cmark, total_ratio);
  printf("\nIterations: %d, Rounds: %d\n", iterations, num_rounds);

  return 0;
}
