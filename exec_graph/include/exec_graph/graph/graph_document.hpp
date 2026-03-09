#pragma once

#include <string>
#include <unordered_map>
#include <vector>

namespace exec_graph::graph {

struct GraphNode {
    std::string id;
    std::vector<std::string> argv;
};

struct GraphEdge {
    std::string from;
    std::string to;
};

struct GraphDocument {
    std::vector<GraphNode> nodes;
    std::vector<GraphEdge> edges;
};

GraphDocument load_graph_document(const std::string& graph_path);
std::vector<std::string> topological_order(const GraphDocument& document);
std::unordered_map<std::string, std::string> execute_linearized_outputs(
    const GraphDocument& document,
    const std::vector<std::string>& execution_order,
    const std::unordered_map<std::string, std::string>& parent_outputs
);

}  // namespace exec_graph::graph
