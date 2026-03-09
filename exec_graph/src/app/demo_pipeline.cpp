#include "exec_graph/graph/graph_document.hpp"
#include "exec_graph/graph_core/graph_snapshot.hpp"
#include "exec_graph/graph_core/graph_repository.hpp"
#include "exec_graph/infra/sqlite/migration_runner.hpp"
#include "exec_graph/runtime/process_runtime.hpp"

#include <chrono>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
namespace {
void print_usage() {
    std::cout << "usage: eg_demo_pipeline "
                 "(--workflow <path> | --graph <path> | --stored-graph <id> --db <path>) "
                 "[--save-graph --graph-id <id> --db <path> --expected-revision <n>] "
                 "[--emit-events-jsonl] "
                 "[--benchmark <iterations>]\n";
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

std::string read_file(const std::string& path) {
    std::ifstream input(path);
    if (!input) {
        throw std::runtime_error("failed to open file: " + path);
    }
    std::string contents((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
    return contents;
}

}  // namespace

int main(int argc, char** argv) {
    try {
        std::string workflow_path;
        std::string graph_path;
        std::string stored_graph_id;
        std::string save_graph_id;
        std::string database_path;
        bool save_graph = false;
        bool emit_events_jsonl = false;
        std::optional<std::int64_t> expected_revision;
        int benchmark_iterations = 0;
        const auto emit_event = [&emit_events_jsonl](const exec_graph::runtime::ProcessEvent& event) {
            if (emit_events_jsonl) {
                std::cout << exec_graph::runtime::render_process_event_json(event) << '\n';
            }
        };

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
            if (arg == "--stored-graph" && i + 1 < argc) {
                stored_graph_id = argv[++i];
                continue;
            }
            if (arg == "--graph-id" && i + 1 < argc) {
                save_graph_id = argv[++i];
                continue;
            }
            if (arg == "--db" && i + 1 < argc) {
                database_path = argv[++i];
                continue;
            }
            if (arg == "--save-graph") {
                save_graph = true;
                continue;
            }
            if (arg == "--emit-events-jsonl") {
                emit_events_jsonl = true;
                continue;
            }
            if (arg == "--expected-revision" && i + 1 < argc) {
                expected_revision = std::stoll(argv[++i]);
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

        int mode_count = 0;
        mode_count += workflow_path.empty() ? 0 : 1;
        mode_count += graph_path.empty() ? 0 : 1;
        mode_count += stored_graph_id.empty() ? 0 : 1;
        if (mode_count != 1) {
            throw std::runtime_error("use exactly one of --workflow, --graph, or --stored-graph");
        }

        if (save_graph) {
            if (graph_path.empty()) {
                throw std::runtime_error("--save-graph requires --graph");
            }
            if (save_graph_id.empty() || database_path.empty()) {
                throw std::runtime_error("--save-graph requires both --graph-id and --db");
            }
        } else if (!save_graph_id.empty() || expected_revision.has_value()) {
            throw std::runtime_error("--graph-id and --expected-revision are only valid with --save-graph");
        }

        if (!stored_graph_id.empty() && database_path.empty()) {
            throw std::runtime_error("--stored-graph requires --db");
        }

        const int iterations = benchmark_iterations > 0 ? benchmark_iterations : 1;

        std::string last_output;
        const auto started = std::chrono::steady_clock::now();
        if (!workflow_path.empty()) {
            const auto workflow = exec_graph::runtime::load_workflow(workflow_path);
            for (int i = 0; i < iterations; ++i) {
                if (emit_events_jsonl) {
                    last_output = exec_graph::runtime::run_workflow(
                        workflow,
                        emit_event
                    );
                } else {
                    last_output = exec_graph::runtime::run_workflow(workflow);
                }
            }
        } else if (!graph_path.empty()) {
            if (save_graph) {
                exec_graph::infra::sqlite::MigrationRunner(database_path, "exec_graph/migrations").apply_all();
                exec_graph::graph_core::GraphRepositorySqlite repository(database_path);
                const auto next_revision =
                    repository.save_graph(save_graph_id, read_file(graph_path), expected_revision);
                std::cout << "stored graph " << save_graph_id << " at revision " << next_revision << "\n";
                return 0;
            }
            const auto document = exec_graph::graph::load_graph_document(graph_path);
            const auto snapshot = exec_graph::graph_core::build_snapshot(document);
            for (int i = 0; i < iterations; ++i) {
                const auto outputs = emit_events_jsonl
                    ? exec_graph::graph_core::execute_snapshot_outputs(
                          *snapshot,
                          emit_event
                      )
                    : exec_graph::graph_core::execute_snapshot_outputs(*snapshot);
                last_output = render_graph_outputs(*snapshot, outputs);
            }
        } else {
            exec_graph::infra::sqlite::MigrationRunner(database_path, "exec_graph/migrations").apply_all();
            exec_graph::graph_core::GraphRepositorySqlite repository(database_path);
            const auto stored = repository.load_graph(stored_graph_id);
            for (int i = 0; i < iterations; ++i) {
                const auto outputs = emit_events_jsonl
                    ? exec_graph::graph_core::execute_snapshot_outputs(
                          *stored.snapshot,
                          emit_event
                      )
                    : exec_graph::graph_core::execute_snapshot_outputs(*stored.snapshot);
                last_output = render_graph_outputs(*stored.snapshot, outputs);
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
