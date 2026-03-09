#include "exec_graph/infra/sqlite/migration_runner.hpp"

#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

std::string default_migrations_dir() {
    namespace fs = std::filesystem;

    const fs::path repo_relative = "exec_graph/migrations";
    if (fs::exists(repo_relative)) {
        return repo_relative.string();
    }

    const fs::path product_relative = "migrations";
    if (fs::exists(product_relative)) {
        return product_relative.string();
    }

    return repo_relative.string();
}

void print_usage() {
    std::cout << "usage: eg_migrate --db <path> [--migrations-dir <path>]\n";
}

}  // namespace

int main(int argc, char** argv) {
    try {
        std::string database_path;
        std::string migrations_dir = default_migrations_dir();

        for (int i = 1; i < argc; ++i) {
            const std::string arg = argv[i];
            if (arg == "--db" && i + 1 < argc) {
                database_path = argv[++i];
                continue;
            }
            if (arg == "--migrations-dir" && i + 1 < argc) {
                migrations_dir = argv[++i];
                continue;
            }
            if (arg == "--help" || arg == "-h") {
                print_usage();
                return 0;
            }
            throw std::runtime_error("unknown or incomplete argument: " + arg);
        }

        if (database_path.empty()) {
            throw std::runtime_error("--db is required");
        }

        const auto summary = exec_graph::infra::sqlite::MigrationRunner(database_path, migrations_dir).apply_all();
        std::cout << "applied migrations from " << migrations_dir << " to " << database_path
                  << " (discovered=" << summary.discovered_count
                  << ", applied=" << summary.applied_count
                  << ", skipped=" << summary.skipped_count << ")\n";
        return 0;
    } catch (const std::exception& exc) {
        std::cerr << "eg_migrate error: " << exc.what() << "\n";
        return 1;
    }
}
