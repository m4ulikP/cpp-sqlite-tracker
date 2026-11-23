#include "repository/ItemRepository.hpp"
#include "sqlite3.h"

#include <iostream>
#include <string>

namespace tracker::repository {

namespace {

class ScopedStatement {
public:
    ScopedStatement() : stmt_(nullptr) {}
    explicit ScopedStatement(sqlite3_stmt* s) : stmt_(s) {}
    ~ScopedStatement() {
        if (stmt_) {
            sqlite3_finalize(stmt_);
            stmt_ = nullptr;
        }
    }

    ScopedStatement(const ScopedStatement&) = delete;
    ScopedStatement& operator=(const ScopedStatement&) = delete;

    ScopedStatement(ScopedStatement&& other) noexcept : stmt_(other.stmt_) {
        other.stmt_ = nullptr;
    }

    ScopedStatement& operator=(ScopedStatement&& other) noexcept {
        if (this != &other) {
            if (stmt_) {
                sqlite3_finalize(stmt_);
            }
            stmt_ = other.stmt_;
            other.stmt_ = nullptr;
        }
        return *this;
    }

    sqlite3_stmt* get() const { return stmt_; }
    sqlite3_stmt** getAddressOf() { return &stmt_; }
    operator sqlite3_stmt*() const { return stmt_; }

private:
    sqlite3_stmt* stmt_ = nullptr;
};

models::Item parseItemRow(sqlite3_stmt* stmt) {
    models::Item item;
    item.id = sqlite3_column_int(stmt, 0);

    const unsigned char* title = sqlite3_column_text(stmt, 1);
    item.title = title ? reinterpret_cast<const char*>(title) : "";

    const unsigned char* type = sqlite3_column_text(stmt, 2);
    item.type = type ? reinterpret_cast<const char*>(type) : "";

    const unsigned char* category = sqlite3_column_text(stmt, 3);
    item.category = category ? reinterpret_cast<const char*>(category) : "";

    item.amount = sqlite3_column_double(stmt, 4);

    const unsigned char* status = sqlite3_column_text(stmt, 5);
    item.status = status ? reinterpret_cast<const char*>(status) : "";

    const unsigned char* priority = sqlite3_column_text(stmt, 6);
    item.priority = priority ? reinterpret_cast<const char*>(priority) : "";

    const unsigned char* dueDate = sqlite3_column_text(stmt, 7);
    item.due_date = dueDate ? reinterpret_cast<const char*>(dueDate) : "";

    const unsigned char* description = sqlite3_column_text(stmt, 8);
    item.description = description ? reinterpret_cast<const char*>(description) : "";

    const unsigned char* createdAt = sqlite3_column_text(stmt, 9);
    item.created_at = createdAt ? reinterpret_cast<const char*>(createdAt) : "";

    const unsigned char* updatedAt = sqlite3_column_text(stmt, 10);
    item.updated_at = updatedAt ? reinterpret_cast<const char*>(updatedAt) : "";

    return item;
}

} // anonymous namespace

ItemRepository::ItemRepository(sqlite3* db) : db_(db) {}

std::vector<models::Item> ItemRepository::findAll(
    const std::optional<std::string>& typeFilter,
    const std::optional<std::string>& statusFilter,
    const std::optional<std::string>& categoryFilter,
    const std::optional<std::string>& search) {

    std::vector<models::Item> items;
    if (!db_) return items;

    std::string sql = "SELECT id, title, type, category, amount, status, priority, due_date, description, created_at, updated_at "
                      "FROM items WHERE 1=1";

    if (typeFilter.has_value() && !typeFilter->empty()) {
        sql += " AND type = ?";
    }
    if (statusFilter.has_value() && !statusFilter->empty()) {
        sql += " AND status = ?";
    }
    if (categoryFilter.has_value() && !categoryFilter->empty()) {
        sql += " AND category = ?";
    }
    if (search.has_value() && !search->empty()) {
        sql += " AND (title LIKE ? OR description LIKE ?)";
    }
    sql += " ORDER BY id DESC;";

    ScopedStatement stmt;
    int rc = sqlite3_prepare_v2(db_, sql.c_str(), -1, stmt.getAddressOf(), nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "[ItemRepository Error] Failed to prepare findAll statement: " 
                  << sqlite3_errmsg(db_) << "\n";
        return items;
    }

    int bindIdx = 1;
    if (typeFilter.has_value() && !typeFilter->empty()) {
        sqlite3_bind_text(stmt, bindIdx++, typeFilter->c_str(), -1, SQLITE_TRANSIENT);
    }
    if (statusFilter.has_value() && !statusFilter->empty()) {
        sqlite3_bind_text(stmt, bindIdx++, statusFilter->c_str(), -1, SQLITE_TRANSIENT);
    }
    if (categoryFilter.has_value() && !categoryFilter->empty()) {
        sqlite3_bind_text(stmt, bindIdx++, categoryFilter->c_str(), -1, SQLITE_TRANSIENT);
    }
    if (search.has_value() && !search->empty()) {
        std::string pattern = "%" + *search + "%";
        sqlite3_bind_text(stmt, bindIdx++, pattern.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, bindIdx++, pattern.c_str(), -1, SQLITE_TRANSIENT);
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        items.push_back(parseItemRow(stmt));
    }

    return items;
}

std::optional<models::Item> ItemRepository::findById(int id) {
    if (!db_) return std::nullopt;

    const char* sql = "SELECT id, title, type, category, amount, status, priority, due_date, description, created_at, updated_at "
                      "FROM items WHERE id = ?;";

    ScopedStatement stmt;
    int rc = sqlite3_prepare_v2(db_, sql, -1, stmt.getAddressOf(), nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "[ItemRepository Error] Failed to prepare findById statement: " 
                  << sqlite3_errmsg(db_) << "\n";
        return std::nullopt;
    }

    sqlite3_bind_int(stmt, 1, id);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        return parseItemRow(stmt);
    }

    return std::nullopt;
}

int ItemRepository::create(const models::Item& item) {
    if (!db_) return -1;

    const char* sql = "INSERT INTO items (title, type, category, amount, status, priority, due_date, description) "
                      "VALUES (?, ?, ?, ?, ?, ?, ?, ?);";

    ScopedStatement stmt;
    int rc = sqlite3_prepare_v2(db_, sql, -1, stmt.getAddressOf(), nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "[ItemRepository Error] Failed to prepare create statement: " 
                  << sqlite3_errmsg(db_) << "\n";
        return -1;
    }

    sqlite3_bind_text(stmt, 1, item.title.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, item.type.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, item.category.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(stmt, 4, item.amount);
    sqlite3_bind_text(stmt, 5, item.status.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 6, item.priority.c_str(), -1, SQLITE_TRANSIENT);
    if (item.due_date.empty()) {
        sqlite3_bind_null(stmt, 7);
    } else {
        sqlite3_bind_text(stmt, 7, item.due_date.c_str(), -1, SQLITE_TRANSIENT);
    }
    sqlite3_bind_text(stmt, 8, item.description.c_str(), -1, SQLITE_TRANSIENT);

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        std::cerr << "[ItemRepository Error] Failed to execute create: " 
                  << sqlite3_errmsg(db_) << "\n";
        return -1;
    }

    return static_cast<int>(sqlite3_last_insert_rowid(db_));
}

bool ItemRepository::update(int id, const models::Item& item) {
    if (!db_) return false;

    const char* sql = "UPDATE items SET title = ?, type = ?, category = ?, amount = ?, status = ?, priority = ?, due_date = ?, description = ? "
                      "WHERE id = ?;";

    ScopedStatement stmt;
    int rc = sqlite3_prepare_v2(db_, sql, -1, stmt.getAddressOf(), nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "[ItemRepository Error] Failed to prepare update statement: " 
                  << sqlite3_errmsg(db_) << "\n";
        return false;
    }

    sqlite3_bind_text(stmt, 1, item.title.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, item.type.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, item.category.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(stmt, 4, item.amount);
    sqlite3_bind_text(stmt, 5, item.status.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 6, item.priority.c_str(), -1, SQLITE_TRANSIENT);
    if (item.due_date.empty()) {
        sqlite3_bind_null(stmt, 7);
    } else {
        sqlite3_bind_text(stmt, 7, item.due_date.c_str(), -1, SQLITE_TRANSIENT);
    }
    sqlite3_bind_text(stmt, 8, item.description.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 9, id);

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        std::cerr << "[ItemRepository Error] Failed to execute update: " 
                  << sqlite3_errmsg(db_) << "\n";
        return false;
    }

    return sqlite3_changes(db_) > 0;
}

bool ItemRepository::remove(int id) {
    if (!db_) return false;

    const char* sql = "DELETE FROM items WHERE id = ?;";

    ScopedStatement stmt;
    int rc = sqlite3_prepare_v2(db_, sql, -1, stmt.getAddressOf(), nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "[ItemRepository Error] Failed to prepare delete statement: " 
                  << sqlite3_errmsg(db_) << "\n";
        return false;
    }

    sqlite3_bind_int(stmt, 1, id);

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        std::cerr << "[ItemRepository Error] Failed to execute delete: " 
                  << sqlite3_errmsg(db_) << "\n";
        return false;
    }

    return sqlite3_changes(db_) > 0;
}

models::ItemSummaryStats ItemRepository::getSummaryStats() {
    models::ItemSummaryStats stats{};
    if (!db_) return stats;

    // Expenses aggregation
    {
        const char* expSql = "SELECT "
                             "COUNT(*), "
                             "COALESCE(SUM(amount), 0.0), "
                             "COALESCE(SUM(CASE WHEN status = 'paid' THEN amount ELSE 0.0 END), 0.0), "
                             "COALESCE(SUM(CASE WHEN status = 'pending' THEN amount ELSE 0.0 END), 0.0) "
                             "FROM items WHERE type = 'expense';";
        ScopedStatement stmt;
        if (sqlite3_prepare_v2(db_, expSql, -1, stmt.getAddressOf(), nullptr) == SQLITE_OK) {
            if (sqlite3_step(stmt) == SQLITE_ROW) {
                stats.expenses.total_count = sqlite3_column_int(stmt, 0);
                stats.expenses.total_amount = sqlite3_column_double(stmt, 1);
                stats.expenses.paid_amount = sqlite3_column_double(stmt, 2);
                stats.expenses.pending_amount = sqlite3_column_double(stmt, 3);
            }
        }
    }

    // Tasks aggregation
    {
        const char* taskSql = "SELECT "
                              "COUNT(*), "
                              "COALESCE(SUM(CASE WHEN status = 'completed' THEN 1 ELSE 0 END), 0), "
                              "COALESCE(SUM(CASE WHEN status = 'in_progress' THEN 1 ELSE 0 END), 0), "
                              "COALESCE(SUM(CASE WHEN status = 'pending' THEN 1 ELSE 0 END), 0) "
                              "FROM items WHERE type = 'task';";
        ScopedStatement stmt;
        if (sqlite3_prepare_v2(db_, taskSql, -1, stmt.getAddressOf(), nullptr) == SQLITE_OK) {
            if (sqlite3_step(stmt) == SQLITE_ROW) {
                stats.tasks.total_count = sqlite3_column_int(stmt, 0);
                stats.tasks.completed_count = sqlite3_column_int(stmt, 1);
                stats.tasks.in_progress_count = sqlite3_column_int(stmt, 2);
                stats.tasks.pending_count = sqlite3_column_int(stmt, 3);
                if (stats.tasks.total_count > 0) {
                    stats.tasks.completion_percentage = 
                        (static_cast<double>(stats.tasks.completed_count) * 100.0) / stats.tasks.total_count;
                } else {
                    stats.tasks.completion_percentage = 0.0;
                }
            }
        }
    }

    return stats;
}

} // namespace tracker::repository
