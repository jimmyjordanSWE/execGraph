#pragma once

#include <string>

namespace exec_graph::infra::sqlite {

class MigrationRunner {
public:
    MigrationRunner(std::string database_path, std::string migrations_dir);

    void apply_all() const;

private:
    std::string database_path_;
    std::string migrations_dir_;
};

}  // namespace exec_graph::infra::sqlite
