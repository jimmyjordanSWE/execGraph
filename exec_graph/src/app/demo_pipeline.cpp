#include "exec_graph/runtime/process_runtime.hpp"

#include <chrono>
#include <iostream>
#include <stdexcept>
#include <string>
namespace {
void print_usage() {
    std::cout << "usage: eg_demo_pipeline --workflow <path> [--benchmark <iterations>]\n";
}

}  // namespace

int main(int argc, char** argv) {
    try {
        std::string workflow_path;
        int benchmark_iterations = 0;

        for (int i = 1; i < argc; ++i) {
            const std::string arg = argv[i];
            if (arg == "--workflow" && i + 1 < argc) {
                workflow_path = argv[++i];
                continue;
            }
            if (arg == "--benchmark" && i + 1 < argc) {
                benchmark_iterations = std::stoi(argv[++i]);
                continue;
            }
            if (arg == "--help" || arg == "-h") {
                print_usage();
                return 0;
            }
            throw std::runtime_error("unknown or incomplete argument: " + arg);
        }

        if (workflow_path.empty()) {
            throw std::runtime_error("missing required --workflow argument");
        }

        const auto workflow = exec_graph::runtime::load_workflow(workflow_path);
        const int iterations = benchmark_iterations > 0 ? benchmark_iterations : 1;

        std::string last_output;
        const auto started = std::chrono::steady_clock::now();
        for (int i = 0; i < iterations; ++i) {
            last_output = exec_graph::runtime::run_workflow(workflow);
        }
        const auto ended = std::chrono::steady_clock::now();
        const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(ended - started);

        std::cout << last_output;
        if (!last_output.empty() && last_output.back() != '\n') {
            std::cout << '\n';
        }

        if (benchmark_iterations > 0) {
            std::cout << "benchmark.iterations=" << iterations << "\n";
            std::cout << "benchmark.total_ms=" << elapsed.count() << "\n";
            std::cout << "benchmark.avg_ms=" << (static_cast<double>(elapsed.count()) / iterations) << "\n";
        }
        return 0;
    } catch (const std::exception& exc) {
        std::cerr << "eg_demo_pipeline error: " << exc.what() << "\n";
        return 1;
    }
}
