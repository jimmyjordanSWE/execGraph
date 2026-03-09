#include "exec_graph/graph_core/graph_snapshot.hpp"

#include "exec_graph/runtime/process_runtime.hpp"

#include <sstream>
#include <stdexcept>
#include <unordered_map>

namespace exec_graph::graph_core {
namespace {

std::unordered_map<std::string, NodeId> build_node_ids(const graph::GraphDocument& document) {
    std::unordered_map<std::string, NodeId> node_ids;
    node_ids.reserve(document.nodes.size());
    for (std::size_t index = 0; index < document.nodes.size(); ++index) {
        node_ids.emplace(document.nodes[index].id, NodeId{static_cast<std::uint32_t>(index)});
    }
    return node_ids;
}

}  // namespace

GraphSnapshot::GraphSnapshot()
    : arena_(),
      working_directory_(&arena_),
      nodes_(&arena_),
      argv_storage_(&arena_),
      incoming_storage_(&arena_),
      outgoing_storage_(&arena_),
      execution_order_(&arena_) {}

std::size_t GraphSnapshot::node_count() const {
    return nodes_.size();
}

const std::pmr::string& GraphSnapshot::working_directory() const {
    return working_directory_;
}

const std::pmr::vector<NodeRecord>& GraphSnapshot::nodes() const {
    return nodes_;
}

const std::pmr::vector<NodeId>& GraphSnapshot::execution_order() const {
    return execution_order_;
}

const NodeRecord& GraphSnapshot::node(NodeId id) const {
    return nodes_.at(id.value);
}

std::vector<std::string> GraphSnapshot::argv_copy(NodeId id) const {
    const auto& record = node(id);
    std::vector<std::string> argv;
    argv.reserve(record.argv_count);
    for (std::size_t index = 0; index < record.argv_count; ++index) {
        argv.push_back(std::string(argv_storage_[record.argv_offset + index]));
    }
    return argv;
}

std::vector<NodeId> GraphSnapshot::incoming(NodeId id) const {
    const auto& record = node(id);
    std::vector<NodeId> edges;
    edges.reserve(record.incoming_count);
    for (std::size_t index = 0; index < record.incoming_count; ++index) {
        edges.push_back(incoming_storage_[record.incoming_offset + index]);
    }
    return edges;
}

std::vector<NodeId> GraphSnapshot::outgoing(NodeId id) const {
    const auto& record = node(id);
    std::vector<NodeId> edges;
    edges.reserve(record.outgoing_count);
    for (std::size_t index = 0; index < record.outgoing_count; ++index) {
        edges.push_back(outgoing_storage_[record.outgoing_offset + index]);
    }
    return edges;
}

std::vector<NodeId> GraphSnapshot::sink_nodes() const {
    std::vector<NodeId> sinks;
    sinks.reserve(nodes_.size());
    for (const auto& record : nodes_) {
        if (record.outgoing_count == 0) {
            sinks.push_back(record.id);
        }
    }
    return sinks;
}

NodeId GraphSnapshot::append_node(const graph::GraphNode& node) {
    const auto id = NodeId{static_cast<std::uint32_t>(nodes_.size())};
    const auto argv_offset = argv_storage_.size();
    for (const auto& arg : node.argv) {
        argv_storage_.emplace_back(arg.c_str());
    }

    nodes_.push_back(NodeRecord{
        id,
        std::pmr::string(node.id.c_str(), &arena_),
        argv_offset,
        node.argv.size(),
        node.timeout_ms,
        node.graceful_shutdown_ms,
        0,
        0,
        0,
        0,
    });
    return id;
}

void GraphSnapshot::reserve_adjacency(const std::size_t total_incoming, const std::size_t total_outgoing) {
    incoming_storage_.reserve(total_incoming);
    outgoing_storage_.reserve(total_outgoing);
}

void GraphSnapshot::set_adjacency(const NodeId id,
                                  const std::vector<NodeId>& incoming_nodes,
                                  const std::vector<NodeId>& outgoing_nodes) {
    auto& record = nodes_.at(id.value);
    record.incoming_offset = incoming_storage_.size();
    record.incoming_count = incoming_nodes.size();
    incoming_storage_.insert(incoming_storage_.end(), incoming_nodes.begin(), incoming_nodes.end());

    record.outgoing_offset = outgoing_storage_.size();
    record.outgoing_count = outgoing_nodes.size();
    outgoing_storage_.insert(outgoing_storage_.end(), outgoing_nodes.begin(), outgoing_nodes.end());
}

void GraphSnapshot::set_execution_order(const std::vector<NodeId>& order) {
    execution_order_.clear();
    execution_order_.reserve(order.size());
    execution_order_.insert(execution_order_.end(), order.begin(), order.end());
}

void GraphSnapshot::set_working_directory(const std::string& working_directory) {
    working_directory_ = std::pmr::string(working_directory.c_str(), &arena_);
}

std::unique_ptr<GraphSnapshot> build_snapshot(const graph::GraphDocument& document) {
    auto snapshot = std::make_unique<GraphSnapshot>();
    snapshot->set_working_directory(document.working_directory);
    snapshot->nodes_.reserve(document.nodes.size());

    std::size_t total_argv = 0;
    for (const auto& node : document.nodes) {
        total_argv += node.argv.size();
    }
    snapshot->argv_storage_.reserve(total_argv);

    std::vector<std::vector<NodeId>> incoming(document.nodes.size());
    std::vector<std::vector<NodeId>> outgoing(document.nodes.size());

    for (const auto& node : document.nodes) {
        snapshot->append_node(node);
    }

    const auto node_ids = build_node_ids(document);
    for (const auto& edge : document.edges) {
        const auto from = node_ids.at(edge.from);
        const auto to = node_ids.at(edge.to);
        outgoing[from.value].push_back(to);
        incoming[to.value].push_back(from);
    }

    snapshot->reserve_adjacency(document.edges.size(), document.edges.size());
    for (std::size_t index = 0; index < document.nodes.size(); ++index) {
        snapshot->set_adjacency(NodeId{static_cast<std::uint32_t>(index)}, incoming[index], outgoing[index]);
    }

    const auto order_names = graph::topological_order(document);
    std::vector<NodeId> order_ids;
    order_ids.reserve(order_names.size());
    for (const auto& name : order_names) {
        order_ids.push_back(node_ids.at(name));
    }
    snapshot->set_execution_order(order_ids);
    return snapshot;
}

std::unordered_map<std::string, std::string> execute_snapshot_outputs(const GraphSnapshot& snapshot) {
    return execute_snapshot_outputs(snapshot, {});
}

std::unordered_map<std::string, std::string> execute_snapshot_outputs(
    const GraphSnapshot& snapshot,
    const std::function<void(const runtime::ProcessEvent&)>& event_sink
) {
    std::unordered_map<std::string, std::string> outputs;
    outputs.reserve(snapshot.node_count());
    const auto sink_count = static_cast<int>(snapshot.sink_nodes().size());
    int completed_node_count = 0;

    if (event_sink) {
        event_sink(runtime::build_graph_started_event(static_cast<int>(snapshot.node_count()), sink_count));
    }

    for (const auto node_id : snapshot.execution_order()) {
        const auto node_name = std::string(snapshot.node(node_id).name);
        std::string stdin_data;
        const auto incoming = snapshot.incoming(node_id);
        if (!incoming.empty()) {
            stdin_data = outputs.at(snapshot.node(incoming.front()).name.c_str());
        }

        if (event_sink) {
            event_sink(
                runtime::build_graph_node_started_event(
                    node_name,
                    static_cast<int>(snapshot.node_count()),
                    sink_count,
                    completed_node_count
                )
            );
        }

        const auto& record = snapshot.node(node_id);
        const runtime::ProcessSpec spec{
            snapshot.argv_copy(node_id),
            std::string(snapshot.working_directory()),
            record.timeout_ms,
            record.graceful_shutdown_ms,
        };
        const auto result = runtime::run_process(node_name, spec, stdin_data, event_sink);
        if (event_sink) {
            event_sink(runtime::build_process_event(node_name, result));
        }
        if (result.exit_code != 0) {
            if (event_sink) {
                event_sink(
                    runtime::build_graph_node_failed_event(
                        node_name,
                        static_cast<int>(snapshot.node_count()),
                        sink_count,
                        completed_node_count,
                        result
                    )
                );
                event_sink(
                    runtime::build_graph_failed_event(
                        static_cast<int>(snapshot.node_count()),
                        sink_count,
                        completed_node_count,
                        node_name,
                        result
                    )
                );
            }
            throw std::runtime_error(
                runtime::format_process_failure(
                    "node " + std::string(snapshot.node(node_id).name),
                    spec,
                    result
                )
            );
        }
        outputs.emplace(node_name, result.stdout_data);
        ++completed_node_count;

        if (event_sink) {
            event_sink(
                runtime::build_graph_node_completed_event(
                    node_name,
                    static_cast<int>(snapshot.node_count()),
                    sink_count,
                    completed_node_count,
                    result
                )
            );
        }
    }

    if (event_sink) {
        event_sink(runtime::build_graph_completed_event(static_cast<int>(snapshot.node_count()), sink_count));
    }

    return outputs;
}

}  // namespace exec_graph::graph_core
