#pragma once

#include "exec_graph/runtime/process_runtime.hpp"

#include "exec_graph/graph/graph_document.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <memory_resource>
#include <string>
#include <functional>
#include <unordered_map>
#include <vector>

namespace exec_graph::graph_core {

struct NodeId {
    std::uint32_t value;
};

struct NodeRecord {
    NodeId id;
    std::pmr::string name;
    std::size_t argv_offset;
    std::size_t argv_count;
    std::size_t incoming_offset;
    std::size_t incoming_count;
    std::size_t outgoing_offset;
    std::size_t outgoing_count;
};

class GraphSnapshot {
public:
    GraphSnapshot();

    GraphSnapshot(const GraphSnapshot&) = delete;
    GraphSnapshot& operator=(const GraphSnapshot&) = delete;
    GraphSnapshot(GraphSnapshot&&) = delete;
    GraphSnapshot& operator=(GraphSnapshot&&) = delete;
    ~GraphSnapshot() = default;

    std::size_t node_count() const;
    const std::pmr::string& working_directory() const;
    const std::pmr::vector<NodeRecord>& nodes() const;
    const std::pmr::vector<NodeId>& execution_order() const;

    const NodeRecord& node(NodeId id) const;
    std::vector<std::string> argv_copy(NodeId id) const;
    std::vector<NodeId> incoming(NodeId id) const;
    std::vector<NodeId> outgoing(NodeId id) const;
    std::vector<NodeId> sink_nodes() const;

private:
    std::pmr::monotonic_buffer_resource arena_;
    std::pmr::string working_directory_;
    std::pmr::vector<NodeRecord> nodes_;
    std::pmr::vector<std::pmr::string> argv_storage_;
    std::pmr::vector<NodeId> incoming_storage_;
    std::pmr::vector<NodeId> outgoing_storage_;
    std::pmr::vector<NodeId> execution_order_;

    NodeId append_node(const graph::GraphNode& node);
    void reserve_adjacency(std::size_t total_incoming, std::size_t total_outgoing);
    void set_adjacency(NodeId id,
                       const std::vector<NodeId>& incoming,
                       const std::vector<NodeId>& outgoing);
    void set_execution_order(const std::vector<NodeId>& order);
    void set_working_directory(const std::string& working_directory);

    friend std::unique_ptr<GraphSnapshot> build_snapshot(const graph::GraphDocument& document);
};

std::unique_ptr<GraphSnapshot> build_snapshot(const graph::GraphDocument& document);
std::unordered_map<std::string, std::string> execute_snapshot_outputs(
    const GraphSnapshot& snapshot
);
std::unordered_map<std::string, std::string> execute_snapshot_outputs(
    const GraphSnapshot& snapshot,
    const std::function<void(const runtime::ProcessEvent&)>& event_sink
);

}  // namespace exec_graph::graph_core
