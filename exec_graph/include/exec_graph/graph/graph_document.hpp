#pragma once

#include <string>
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

}  // namespace exec_graph::graph
