#pragma once

#include <string>

namespace exec_graph::infra::sqlite {

struct MigrationSummary {
    int discovered_count;
    int applied_count;
    int skipped_count;
};

class MigrationRunner {
public:
    MigrationRunner(std::string database_path, std::string migrations_dir);

    MigrationSummary apply_all() const;

private:
    std::string database_path_;
    std::string migrations_dir_;
};

}  // namespace exec_graph::infra::sqlite
