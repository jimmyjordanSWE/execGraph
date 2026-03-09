#include "exec_graph/infra/sqlite/connection.hpp"

#include <stdexcept>
#include <string>

struct sqlite3;
struct sqlite3_stmt;

extern "C" {
int sqlite3_open_v2(const char* filename, sqlite3** ppDb, int flags, const char* zVfs);
int sqlite3_close(sqlite3*);
int sqlite3_exec(sqlite3*, const char* sql, int (*callback)(void*, int, char**, char**), void*, char** errmsg);
void sqlite3_free(void*);
const char* sqlite3_errmsg(sqlite3*);
int sqlite3_prepare_v2(sqlite3*, const char* zSql, int nByte, sqlite3_stmt** ppStmt, const char** pzTail);
int sqlite3_bind_text(sqlite3_stmt*, int, const char*, int n, void (*)(void*));
int sqlite3_bind_int64(sqlite3_stmt*, int, long long);
int sqlite3_step(sqlite3_stmt*);
int sqlite3_finalize(sqlite3_stmt* pStmt);
int sqlite3_reset(sqlite3_stmt* pStmt);
int sqlite3_busy_timeout(sqlite3*, int ms);
const unsigned char* sqlite3_column_text(sqlite3_stmt*, int iCol);
long long sqlite3_column_int64(sqlite3_stmt*, int iCol);
}

namespace exec_graph::infra::sqlite {
namespace {

constexpr int kSqliteOk = 0;
constexpr int kSqliteRow = 100;
constexpr int kSqliteDone = 101;
constexpr int kSqliteOpenReadWrite = 0x00000002;
constexpr int kSqliteOpenCreate = 0x00000004;
constexpr int kSqliteTransient = -1;

[[noreturn]] void throw_sqlite_error(sqlite3* handle, const std::string& prefix) {
    throw std::runtime_error(prefix + ": " + sqlite3_errmsg(handle));
}

void check_result(sqlite3* handle, const int rc, const std::string& prefix) {
    if (rc != kSqliteOk) {
        throw_sqlite_error(handle, prefix);
    }
}

}  // namespace

Statement::Statement(sqlite3_stmt* handle) : handle_(handle) {}

Statement::Statement(Statement&& other) noexcept : handle_(other.handle_) {
    other.handle_ = nullptr;
}

Statement& Statement::operator=(Statement&& other) noexcept {
    if (this == &other) {
        return *this;
    }
    if (handle_ != nullptr) {
        sqlite3_finalize(handle_);
    }
    handle_ = other.handle_;
    other.handle_ = nullptr;
    return *this;
}

Statement::~Statement() {
    if (handle_ != nullptr) {
        sqlite3_finalize(handle_);
    }
}

void Statement::bind_text(const int index, const std::string& value) {
    if (sqlite3_bind_text(handle_, index, value.c_str(), static_cast<int>(value.size()), reinterpret_cast<void (*)(void*)>(kSqliteTransient)) != kSqliteOk) {
        throw std::runtime_error("failed to bind sqlite text parameter");
    }
}

void Statement::bind_int64(const int index, const std::int64_t value) {
    if (sqlite3_bind_int64(handle_, index, value) != kSqliteOk) {
        throw std::runtime_error("failed to bind sqlite int64 parameter");
    }
}

bool Statement::step_row() {
    const int rc = sqlite3_step(handle_);
    if (rc == kSqliteRow) {
        return true;
    }
    if (rc == kSqliteDone) {
        return false;
    }
    throw std::runtime_error("sqlite statement step failed");
}

void Statement::step_done() {
    const int rc = sqlite3_step(handle_);
    if (rc != kSqliteDone) {
        throw std::runtime_error("sqlite statement did not complete");
    }
}

void Statement::reset() {
    sqlite3_reset(handle_);
}

std::string Statement::column_text(const int index) const {
    const auto* value = sqlite3_column_text(handle_, index);
    return value == nullptr ? std::string{} : std::string(reinterpret_cast<const char*>(value));
}

std::int64_t Statement::column_int64(const int index) const {
    return static_cast<std::int64_t>(sqlite3_column_int64(handle_, index));
}

Connection::Connection(const std::string& path) : handle_(nullptr), in_transaction_(false) {
    const int rc = sqlite3_open_v2(path.c_str(), &handle_, kSqliteOpenReadWrite | kSqliteOpenCreate, nullptr);
    if (rc != kSqliteOk) {
        throw_sqlite_error(handle_, "failed to open sqlite database");
    }
    check_result(handle_, sqlite3_busy_timeout(handle_, 5000), "failed to set sqlite busy timeout");
    execute("PRAGMA foreign_keys = ON");
}

Connection::~Connection() {
    if (handle_ != nullptr) {
        sqlite3_close(handle_);
    }
}

void Connection::execute(const std::string& sql) {
    char* error_message = nullptr;
    const int rc = sqlite3_exec(handle_, sql.c_str(), nullptr, nullptr, &error_message);
    if (rc != kSqliteOk) {
        const std::string error = error_message == nullptr ? sqlite3_errmsg(handle_) : std::string(error_message);
        if (error_message != nullptr) {
            sqlite3_free(error_message);
        }
        throw std::runtime_error("sqlite exec failed: " + error);
    }
}

Statement Connection::prepare(const std::string& sql) const {
    sqlite3_stmt* statement = nullptr;
    check_result(handle_, sqlite3_prepare_v2(handle_, sql.c_str(), -1, &statement, nullptr), "failed to prepare sqlite statement");
    return Statement(statement);
}

void Connection::begin_immediate() {
    execute("BEGIN IMMEDIATE");
    in_transaction_ = true;
}

void Connection::commit() {
    execute("COMMIT");
    in_transaction_ = false;
}

void Connection::rollback() {
    execute("ROLLBACK");
    in_transaction_ = false;
}

bool Connection::in_transaction() const {
    return in_transaction_;
}

Transaction::Transaction(Connection& connection) : connection_(connection), committed_(false) {
    connection_.begin_immediate();
}

Transaction::~Transaction() {
    if (!committed_ && connection_.in_transaction()) {
        try {
            connection_.rollback();
        } catch (...) {
        }
    }
}

void Transaction::commit() {
    if (!committed_) {
        connection_.commit();
        committed_ = true;
    }
}

}  // namespace exec_graph::infra::sqlite
