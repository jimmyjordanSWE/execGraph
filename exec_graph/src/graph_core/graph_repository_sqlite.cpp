#include "exec_graph/graph_core/graph_repository.hpp"

#include "exec_graph/graph/graph_document.hpp"
#include "exec_graph/infra/sqlite/connection.hpp"

#include <fstream>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace exec_graph::graph_core {
namespace {

std::string load_existing_graph_source(infra::sqlite::Connection& connection,
                                       const std::string& graph_id,
                                       std::int64_t* revision_out) {
    auto select = connection.prepare(
        "SELECT revision, source_text "
        "FROM graph_snapshots "
        "WHERE graph_id = ?1"
    );
    select.bind_text(1, graph_id);
    if (!select.step_row()) {
        return {};
    }

    *revision_out = select.column_int64(0);
    return select.column_text(1);
}

}  // namespace

GraphRepositorySqlite::GraphRepositorySqlite(const std::string& database_path)
    : database_path_(database_path) {}

std::int64_t GraphRepositorySqlite::save_graph(const std::string& graph_id,
                                               const std::string& graph_source,
                                               const std::optional<std::int64_t> expected_revision) {
    const auto document = graph::load_graph_document_from_string(graph_source, "stored graph " + graph_id);
    (void)build_snapshot(document);

    infra::sqlite::Connection connection(database_path_);
    connection.begin_immediate();
    try {
        std::int64_t current_revision = 0;
        const auto existing_source = load_existing_graph_source(connection, graph_id, &current_revision);

        std::int64_t next_revision = 1;
        if (!existing_source.empty()) {
            if (!expected_revision.has_value()) {
                throw std::runtime_error("revision_conflict: expected_revision is required to update existing graph " + graph_id);
            }
            if (expected_revision.value() != current_revision) {
                throw std::runtime_error(
                    "revision_conflict: graph " + graph_id + " is at revision " + std::to_string(current_revision) +
                    ", expected " + std::to_string(expected_revision.value())
                );
            }
            next_revision = current_revision + 1;
            auto update = connection.prepare(
                "UPDATE graph_snapshots "
                "SET revision = ?1, source_text = ?2, updated_at = CURRENT_TIMESTAMP "
                "WHERE graph_id = ?3"
            );
            update.bind_int64(1, next_revision);
            update.bind_text(2, graph_source);
            update.bind_text(3, graph_id);
            update.step_done();
        } else {
            if (expected_revision.has_value()) {
                throw std::runtime_error("revision_conflict: graph " + graph_id + " does not exist yet");
            }
            auto insert = connection.prepare(
                "INSERT INTO graph_snapshots (graph_id, revision, source_text) "
                "VALUES (?1, ?2, ?3)"
            );
            insert.bind_text(1, graph_id);
            insert.bind_int64(2, next_revision);
            insert.bind_text(3, graph_source);
            insert.step_done();
        }

        connection.commit();
        return next_revision;
    } catch (...) {
        connection.rollback();
        throw;
    }
}

StoredGraph GraphRepositorySqlite::load_graph(const std::string& graph_id) {
    infra::sqlite::Connection connection(database_path_);
    auto select = connection.prepare(
        "SELECT revision, source_text "
        "FROM graph_snapshots "
        "WHERE graph_id = ?1"
    );
    select.bind_text(1, graph_id);
    if (!select.step_row()) {
        throw std::runtime_error("graph not found: " + graph_id);
    }

    StoredGraph stored{
        graph_id,
        select.column_int64(0),
        select.column_text(1),
        nullptr,
    };

    const auto document =
        graph::load_graph_document_from_string(stored.source_text, "stored graph " + graph_id);
    stored.snapshot = build_snapshot(document);
    return stored;
}

}  // namespace exec_graph::graph_core
