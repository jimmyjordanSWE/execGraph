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
GraphDocument load_graph_document_from_string(const std::string& graph_text, const std::string& source_label);
std::vector<std::string> topological_order(const GraphDocument& document);

}  // namespace exec_graph::graph
