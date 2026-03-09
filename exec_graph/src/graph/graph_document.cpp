#include "exec_graph/graph/graph_document.hpp"

#include <filesystem>
#include <fstream>
#include <queue>
#include <sstream>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>

namespace exec_graph::graph {
namespace {

constexpr int kDefaultGracefulShutdownMs = 250;

std::vector<std::string> split_whitespace(const std::string& line) {
    std::istringstream stream(line);
    std::vector<std::string> parts;
    for (std::string part; stream >> part;) {
        parts.push_back(part);
    }
    return parts;
}

bool starts_with(const std::string& text, const std::string& prefix) {
    return text.rfind(prefix, 0) == 0;
}

int parse_non_negative_int(const std::string& text, const std::string& label) {
    try {
        const int value = std::stoi(text);
        if (value < 0) {
            throw std::runtime_error(label + " must be non-negative");
        }
        return value;
    } catch (const std::invalid_argument&) {
        throw std::runtime_error(label + " must be an integer");
    } catch (const std::out_of_range&) {
        throw std::runtime_error(label + " is out of range");
    }
}

GraphNode parse_node_tokens(const std::vector<std::string>& tokens) {
    if (tokens.size() < 3) {
        throw std::runtime_error("node line requires id and command");
    }

    GraphNode node;
    node.id = tokens[1];
    node.timeout_ms = 0;
    node.graceful_shutdown_ms = kDefaultGracefulShutdownMs;

    std::size_t cursor = 2;
    while (cursor < tokens.size()) {
        const auto& token = tokens[cursor];
        if (starts_with(token, "timeout_ms=")) {
            node.timeout_ms = parse_non_negative_int(token.substr(std::string("timeout_ms=").size()), "timeout_ms");
            ++cursor;
            continue;
        }
        if (starts_with(token, "graceful_shutdown_ms=")) {
            node.graceful_shutdown_ms = parse_non_negative_int(
                token.substr(std::string("graceful_shutdown_ms=").size()),
                "graceful_shutdown_ms"
            );
            ++cursor;
            continue;
        }
        break;
    }

    if (cursor >= tokens.size()) {
        throw std::runtime_error("node line requires a runnable command");
    }

    node.argv.assign(tokens.begin() + static_cast<std::ptrdiff_t>(cursor), tokens.end());
    return node;
}

std::unordered_map<std::string, const GraphNode*> node_index(const GraphDocument& document) {
    std::unordered_map<std::string, const GraphNode*> index;
    for (const auto& node : document.nodes) {
        const auto [_, inserted] = index.emplace(node.id, &node);
        if (!inserted) {
            throw std::runtime_error("duplicate node id: " + node.id);
        }
    }
    return index;
}

std::unordered_map<std::string, std::vector<std::string>> incoming_edges(const GraphDocument& document) {
    std::unordered_map<std::string, std::vector<std::string>> incoming;
    for (const auto& node : document.nodes) {
        incoming.emplace(node.id, std::vector<std::string>{});
    }
    for (const auto& edge : document.edges) {
        incoming[edge.to].push_back(edge.from);
    }
    return incoming;
}

std::unordered_map<std::string, std::vector<std::string>> outgoing_edges(const GraphDocument& document) {
    std::unordered_map<std::string, std::vector<std::string>> outgoing;
    for (const auto& node : document.nodes) {
        outgoing.emplace(node.id, std::vector<std::string>{});
    }
    for (const auto& edge : document.edges) {
        outgoing[edge.from].push_back(edge.to);
    }
    return outgoing;
}

}  // namespace

GraphDocument load_graph_document_from_string(const std::string& graph_text, const std::string& source_label) {
    std::istringstream input(graph_text);
    GraphDocument document;
    for (std::string line; std::getline(input, line);) {
        if (line.empty() || line[0] == '#') {
            continue;
        }
        auto tokens = split_whitespace(line);
        if (tokens.empty()) {
            continue;
        }
        if (tokens[0] == "node") {
            document.nodes.push_back(parse_node_tokens(tokens));
            continue;
        }
        if (tokens[0] == "edge") {
            if (tokens.size() != 3) {
                throw std::runtime_error("edge line requires from and to ids");
            }
            document.edges.push_back(GraphEdge{tokens[1], tokens[2]});
            continue;
        }
        throw std::runtime_error("unknown graph directive: " + tokens[0]);
    }

    if (document.nodes.empty()) {
        throw std::runtime_error(source_label + " contained no nodes");
    }

    const auto index = node_index(document);
    std::unordered_set<std::string> seen_edges;
    auto incoming = incoming_edges(document);
    for (const auto& edge : document.edges) {
        if (!index.count(edge.from)) {
            throw std::runtime_error("edge references unknown source node: " + edge.from);
        }
        if (!index.count(edge.to)) {
            throw std::runtime_error("edge references unknown destination node: " + edge.to);
        }
        const auto edge_key = edge.from + "->" + edge.to;
        if (!seen_edges.emplace(edge_key).second) {
            throw std::runtime_error("duplicate edge: " + edge_key);
        }
        if (incoming[edge.to].size() > 1) {
            throw std::runtime_error("multiple inbound edges are not supported yet for node: " + edge.to);
        }
    }

    (void)topological_order(document);
    return document;
}

GraphDocument load_graph_document(const std::string& graph_path) {
    std::ifstream input(graph_path);
    if (!input) {
        throw std::runtime_error("failed to open graph file: " + graph_path);
    }

    std::ostringstream buffer;
    buffer << input.rdbuf();
    auto document = load_graph_document_from_string(buffer.str(), "graph file " + graph_path);
    document.working_directory = std::filesystem::absolute(std::filesystem::path(graph_path)).parent_path().string();
    return document;
}

std::vector<std::string> topological_order(const GraphDocument& document) {
    const auto index = node_index(document);
    auto indegree = incoming_edges(document);
    auto outgoing = outgoing_edges(document);

    std::queue<std::string> ready;
    for (const auto& [node_id, parents] : indegree) {
        if (parents.empty()) {
            ready.push(node_id);
        }
    }

    std::vector<std::string> order;
    std::unordered_map<std::string, int> remaining_incoming;
    for (const auto& [node_id, parents] : indegree) {
        remaining_incoming[node_id] = static_cast<int>(parents.size());
    }

    while (!ready.empty()) {
        auto node_id = ready.front();
        ready.pop();
        order.push_back(node_id);
        for (const auto& child : outgoing[node_id]) {
            auto& remaining = remaining_incoming[child];
            --remaining;
            if (remaining == 0) {
                ready.push(child);
            }
        }
    }

    if (order.size() != index.size()) {
        throw std::runtime_error("graph contains a cycle or unreachable dependency state");
    }
    return order;
}

}  // namespace exec_graph::graph
