#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#ifdef _WIN32
#include <direct.h>
#include <windows.h>
#else
#include <dirent.h>
#endif

#include "benchmark/benchmark.h"
#include "markus.h"

// cmark-gfm headers
#include "cmark-gfm.h"

// md4c headers
#include "md4c-html.h"
#include "md4c.h"

namespace {

std::string ReadFile(const std::string& path) {
  std::ifstream ifs(path);
  return std::string((std::istreambuf_iterator<char>(ifs)),
                     (std::istreambuf_iterator<char>()));
}

std::vector<std::string> GetSampleFiles(const std::string& dir) {
  std::vector<std::string> files;
#ifdef _WIN32
  std::string pattern = dir + "\\*.md";
  WIN32_FIND_DATAA findData;
  HANDLE hFind = FindFirstFileA(pattern.c_str(), &findData);
  if (hFind == INVALID_HANDLE_VALUE) return files;
  do {
    std::string name = findData.cFileName;
    if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
      files.push_back(dir + "\\" + name);
    }
  } while (FindNextFileA(hFind, &findData));
  FindClose(hFind);
#else
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
#endif
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

// Timing data collected by benchmark lambdas via SetIterationTime
std::mutex g_timing_mutex;
std::map<std::string, std::vector<double>> g_markus_times;
std::map<std::string, std::vector<double>> g_cmark_gfm_times;
std::map<std::string, std::vector<double>> g_md4c_times;

void RecordTiming(const std::string& name, double time_ms,
                  std::map<std::string, std::vector<double>>& map) {
  std::lock_guard<std::mutex> lock(g_timing_mutex);
  map[name].push_back(time_ms);
}

double CalcMean(const std::vector<double>& data) {
  if (data.empty()) return 0.0;
  double sum = 0.0;
  for (auto v : data) sum += v;
  return sum / data.size();
}

void PrintSummary() {
  if (g_markus_times.empty()) return;

  std::map<std::string, bool> all_files;
  for (const auto& [file, _] : g_markus_times) all_files[file] = true;
  for (const auto& [file, _] : g_cmark_gfm_times) all_files[file] = true;
  for (const auto& [file, _] : g_md4c_times) all_files[file] = true;

  printf("\n");
  printf("%-30s %12s %12s %12s %8s %8s\n", "Sample", "Markus(ms)",
         "CmarkGfm(ms)", "Md4c(ms)", "M/GFM", "M/D");
  printf("%-30s %12s %12s %12s %8s %8s\n", "------------------------------",
         "------------", "------------", "------------", "--------",
         "--------");

  double total_markus = 0, total_cmark_gfm = 0, total_md4c = 0;

  for (const auto& [file, _] : all_files) {
    auto get_mean = [](const std::map<std::string, std::vector<double>>& m,
                       const std::string& k) -> double {
      auto it = m.find(k);
      if (it == m.end() || it->second.empty()) return 0.0;
      return CalcMean(it->second);
    };

    double m = get_mean(g_markus_times, file);
    double c = get_mean(g_cmark_gfm_times, file);
    double d = get_mean(g_md4c_times, file);

    double ratio_mc = c > 0 ? m / c : 0;
    double ratio_md = d > 0 ? m / d : 0;

    printf("%-30s %12.3f %12.3f %12.3f %7.2fx %7.2fx\n", file.c_str(), m, c, d,
           ratio_mc, ratio_md);

    total_markus += m;
    total_cmark_gfm += c;
    total_md4c += d;
  }

  printf("%-30s %12s %12s %12s %8s %8s\n", "------------------------------",
         "------------", "------------", "------------", "--------",
         "--------");

  double ratio_mc = total_cmark_gfm > 0 ? total_markus / total_cmark_gfm : 0;
  double ratio_md = total_md4c > 0 ? total_markus / total_md4c : 0;
  printf("%-30s %12.3f %12.3f %12.3f %7.2fx %7.2fx\n", "TOTAL", total_markus,
         total_cmark_gfm, total_md4c, ratio_mc, ratio_md);
  printf("\n");
}

class SummaryReporter : public benchmark::BenchmarkReporter {
 public:
  SummaryReporter()
      : console_reporter_(std::make_unique<benchmark::ConsoleReporter>()) {}

  bool ReportContext(const Context& context) override {
    return console_reporter_->ReportContext(context);
  }

  void ReportRuns(const std::vector<Run>& runs) override {
    console_reporter_->ReportRuns(runs);
  }

  void Finalize() override {
    console_reporter_->Finalize();
    PrintSummary();
  }

 private:
  std::unique_ptr<BenchmarkReporter> console_reporter_;
};

void RegisterBenchmarks(
    const std::vector<std::pair<std::string, std::string>>& inputs) {
  for (const auto& [path, content] : inputs) {
    std::string name = path;
    auto slash = name.rfind('/');
    if (slash != std::string::npos) {
      name = name.substr(slash + 1);
    }

    std::string markus_content = content;
    benchmark::RegisterBenchmark(
        (name + "_markus").c_str(),
        [name, markus_content](benchmark::State& st) {
          for (auto _ : st) {
            auto start = std::chrono::high_resolution_clock::now();
            auto result = markus::MarkdownToHtml(markus_content);
            benchmark::DoNotOptimize(result);
            auto end = std::chrono::high_resolution_clock::now();
            double seconds = std::chrono::duration<double>(end - start).count();
            st.SetIterationTime(seconds);
            RecordTiming(name, seconds * 1000.0, g_markus_times);
          }
          if (st.iterations() > 0) {
            st.SetBytesProcessed(static_cast<int64_t>(st.iterations()) *
                                 markus_content.size());
          }
        });

    std::string cmark_gfm_content = content;
    benchmark::RegisterBenchmark(
        (name + "_cmark_gfm").c_str(),
        [name, cmark_gfm_content](benchmark::State& st) {
          for (auto _ : st) {
            auto start = std::chrono::high_resolution_clock::now();
            cmark_node* doc = cmark_parse_document(cmark_gfm_content.c_str(),
                                                   cmark_gfm_content.size(),
                                                   CMARK_OPT_DEFAULT);
            char* html = cmark_render_html(doc, CMARK_OPT_DEFAULT, nullptr);
            benchmark::DoNotOptimize(html);
            free(html);
            cmark_node_free(doc);
            auto end = std::chrono::high_resolution_clock::now();
            double seconds = std::chrono::duration<double>(end - start).count();
            st.SetIterationTime(seconds);
            RecordTiming(name, seconds * 1000.0, g_cmark_gfm_times);
          }
          if (st.iterations() > 0) {
            st.SetBytesProcessed(static_cast<int64_t>(st.iterations()) *
                                 cmark_gfm_content.size());
          }
        });

    std::string md4c_content = content;
    benchmark::RegisterBenchmark(
        (name + "_md4c").c_str(), [name, md4c_content](benchmark::State& st) {
          for (auto _ : st) {
            auto start = std::chrono::high_resolution_clock::now();
            std::vector<char> output;
            output.reserve(4096);

            md_html(
                md4c_content.c_str(), static_cast<MD_SIZE>(md4c_content.size()),
                [](const char* text, MD_SIZE sz, void* userdata) {
                  auto& vec = *static_cast<std::vector<char>*>(userdata);
                  vec.insert(vec.end(), text, text + sz);
                },
                &output, MD_DIALECT_COMMONMARK, 0);

            benchmark::DoNotOptimize(output.data());
            auto end = std::chrono::high_resolution_clock::now();
            double seconds = std::chrono::duration<double>(end - start).count();
            st.SetIterationTime(seconds);
            RecordTiming(name, seconds * 1000.0, g_md4c_times);
          }
          if (st.iterations() > 0) {
            st.SetBytesProcessed(static_cast<int64_t>(st.iterations()) *
                                 md4c_content.size());
          }
        });
  }
}

}  // namespace

int main(int argc, char** argv) {
  std::string samples_dir = "cmark-gfm/bench/samples";
  std::string input_file;
  int num_rounds = 2;

  SummaryReporter reporter;

  std::vector<std::string> bench_argv_strs;
  std::vector<char*> bench_argv;

  bench_argv_strs.push_back(argv[0]);
  bench_argv.push_back(const_cast<char*>(bench_argv_strs.back().c_str()));

  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    if (arg == "--help" || arg == "-h") {
      std::cerr << "Usage: bench [--input FILE] [--rounds N] [SAMPLES_DIR]\n";
      std::cerr
          << "\nBenchmarks markus vs cmark-gfm vs md4c on markdown samples.\n";
      std::cerr << "\nOptions:\n";
      std::cerr << "  --rounds N       Number of benchmark repetitions "
                   "(default: 2)\n";
      std::cerr << "  --input FILE     Benchmark a single file instead of "
                   "samples dir\n";
      std::cerr << "  SAMPLES_DIR      Directory with .md files (default: "
                   "cmark-gfm/bench/samples)\n";
      std::cerr << "\nGoogle Benchmark options:\n";
      std::cerr << "  --benchmark_min_time=N   Minimum time per benchmark run "
                   "(default: 1s)\n";
      std::cerr
          << "  --benchmark_filter=REGEX Run only benchmarks matching REGEX\n";
      std::cerr << "  --benchmark_list_tests   List benchmark names and exit\n";
      std::cerr << "  --benchmark_format=FORMAT Output format: console, json, "
                   "csv (default: console)\n";
      return 0;
    } else if (arg == "--input" || arg == "-f") {
      ++i;
      if (i < argc) input_file = argv[i];
    } else if (arg == "--rounds" || arg == "-r") {
      ++i;
      if (i < argc) num_rounds = std::atoi(argv[i]);
    } else if (arg[0] != '-') {
      samples_dir = arg;
    } else {
      bench_argv_strs.push_back(arg);
      bench_argv.push_back(const_cast<char*>(bench_argv_strs.back().c_str()));
    }
  }

  auto inputs = CollectInputs(samples_dir, input_file);

  if (inputs.empty()) {
    std::cerr << "No input files found\n";
    return 1;
  }

  RegisterBenchmarks(inputs);

  // Add --benchmark_repetitions for our --rounds option
  std::string reps_arg =
      "--benchmark_repetitions=" + std::to_string(num_rounds);
  bench_argv_strs.push_back(reps_arg);
  bench_argv.push_back(const_cast<char*>(bench_argv_strs.back().c_str()));

  int filtered_argc = static_cast<int>(bench_argv.size());
  benchmark::Initialize(&filtered_argc, bench_argv.data());
  benchmark::RunSpecifiedBenchmarks(&reporter);
  benchmark::Shutdown();
  return 0;
}
