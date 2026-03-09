#include "exec_graph/infra/sqlite/migration_runner.hpp"

#include "exec_graph/infra/sqlite/connection.hpp"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <algorithm>
#include <vector>

namespace exec_graph::infra::sqlite {
namespace {

std::string read_file(const std::filesystem::path& path) {
    std::ifstream input(path);
    if (!input) {
        throw std::runtime_error("failed to open migration file: " + path.string());
    }
    std::ostringstream buffer;
    buffer << input.rdbuf();
    return buffer.str();
}

}  // namespace

MigrationRunner::MigrationRunner(std::string database_path, std::string migrations_dir)
    : database_path_(std::move(database_path)), migrations_dir_(std::move(migrations_dir)) {}

void MigrationRunner::apply_all() const {
    namespace fs = std::filesystem;

    if (!fs::exists(migrations_dir_)) {
        throw std::runtime_error("migration directory does not exist: " + migrations_dir_);
    }

    infra::sqlite::Connection connection(database_path_);
    connection.execute(
        "CREATE TABLE IF NOT EXISTS schema_migrations ("
        "  version TEXT PRIMARY KEY,"
        "  applied_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP"
        ")"
    );

    std::vector<fs::path> migration_files;
    for (const auto& entry : fs::directory_iterator(migrations_dir_)) {
        if (entry.is_regular_file() && entry.path().extension() == ".sql") {
            migration_files.push_back(entry.path());
        }
    }
    std::sort(migration_files.begin(), migration_files.end());

    for (const auto& path : migration_files) {
        const auto version = path.filename().string();

        auto select = connection.prepare(
            "SELECT version FROM schema_migrations WHERE version = ?1"
        );
        select.bind_text(1, version);
        if (select.step_row()) {
            continue;
        }

        connection.begin_immediate();
        try {
            connection.execute(read_file(path));
            auto insert = connection.prepare(
                "INSERT INTO schema_migrations (version) VALUES (?1)"
            );
            insert.bind_text(1, version);
            insert.step_done();
            connection.commit();
        } catch (...) {
            connection.rollback();
            throw;
        }
    }
}

}  // namespace exec_graph::infra::sqlite
