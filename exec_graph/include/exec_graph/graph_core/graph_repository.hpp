#pragma once

#include "exec_graph/graph_core/graph_snapshot.hpp"

#include <cstdint>
#include <memory>
#include <optional>
#include <string>

namespace exec_graph::graph_core {

struct StoredGraph {
    std::string graph_id;
    std::int64_t revision;
    std::string source_text;
    std::unique_ptr<GraphSnapshot> snapshot;
};

class GraphRepositorySqlite {
public:
    explicit GraphRepositorySqlite(const std::string& database_path);

    std::int64_t save_graph(const std::string& graph_id,
                            const std::string& graph_source,
                            const std::string& working_directory,
                            std::optional<std::int64_t> expected_revision);
    StoredGraph load_graph(const std::string& graph_id);

private:
    std::string database_path_;
};

}  // namespace exec_graph::graph_core
