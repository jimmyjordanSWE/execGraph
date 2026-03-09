#include "exec_graph/graph/graph_document.hpp"
#include "exec_graph/graph_core/graph_snapshot.hpp"
#include "exec_graph/runtime/process_runtime.hpp"

#include <chrono>
#include <iostream>
#include <stdexcept>
#include <string>
namespace {
void print_usage() {
    std::cout << "usage: eg_demo_pipeline (--workflow <path> | --graph <path>) [--benchmark <iterations>]\n";
}

std::string render_graph_outputs(const exec_graph::graph_core::GraphSnapshot& snapshot,
                                 const std::unordered_map<std::string, std::string>& outputs) {
    std::string rendered;
    for (const auto sink_id : snapshot.sink_nodes()) {
        const auto& sink = snapshot.node(sink_id);
        rendered += "sink " + std::string(sink.name) + ":\n";
        rendered += outputs.at(std::string(sink.name));
        if (rendered.empty() || rendered.back() != '\n') {
            rendered += '\n';
        }
    }
    return rendered;
}

}  // namespace

int main(int argc, char** argv) {
    try {
        std::string workflow_path;
        std::string graph_path;
        int benchmark_iterations = 0;

        for (int i = 1; i < argc; ++i) {
            const std::string arg = argv[i];
            if (arg == "--workflow" && i + 1 < argc) {
                workflow_path = argv[++i];
                continue;
            }
            if (arg == "--graph" && i + 1 < argc) {
                graph_path = argv[++i];
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

        if (workflow_path.empty() == graph_path.empty()) {
            throw std::runtime_error("use exactly one of --workflow or --graph");
        }

        const int iterations = benchmark_iterations > 0 ? benchmark_iterations : 1;

        std::string last_output;
        const auto started = std::chrono::steady_clock::now();
        if (!workflow_path.empty()) {
            const auto workflow = exec_graph::runtime::load_workflow(workflow_path);
            for (int i = 0; i < iterations; ++i) {
                last_output = exec_graph::runtime::run_workflow(workflow);
            }
        } else {
            const auto document = exec_graph::graph::load_graph_document(graph_path);
            const auto snapshot = exec_graph::graph_core::build_snapshot(document);
            for (int i = 0; i < iterations; ++i) {
                const auto outputs = exec_graph::graph_core::execute_snapshot_outputs(*snapshot);
                last_output = render_graph_outputs(*snapshot, outputs);
            }
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
