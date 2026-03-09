#pragma once

#include <cstdint>
#include <string>

struct sqlite3;
struct sqlite3_stmt;

namespace exec_graph::infra::sqlite {

class Statement {
public:
    explicit Statement(sqlite3_stmt* handle);

    Statement(const Statement&) = delete;
    Statement& operator=(const Statement&) = delete;
    Statement(Statement&& other) noexcept;
    Statement& operator=(Statement&& other) noexcept;
    ~Statement();

    void bind_text(int index, const std::string& value);
    void bind_int64(int index, std::int64_t value);
    bool step_row();
    void step_done();
    void reset();
    std::string column_text(int index) const;
    std::int64_t column_int64(int index) const;

private:
    sqlite3_stmt* handle_;
};

class Connection {
public:
    explicit Connection(const std::string& path);

    Connection(const Connection&) = delete;
    Connection& operator=(const Connection&) = delete;
    Connection(Connection&&) = delete;
    Connection& operator=(Connection&&) = delete;
    ~Connection();

    void execute(const std::string& sql);
    Statement prepare(const std::string& sql) const;
    void begin_immediate();
    void commit();
    void rollback();

private:
    sqlite3* handle_;
};

}  // namespace exec_graph::infra::sqlite
