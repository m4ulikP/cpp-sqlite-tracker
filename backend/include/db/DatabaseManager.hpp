#pragma once

#include <string>
#include <memory>

struct sqlite3;

namespace tracker::db {

class DatabaseManager {
public:
    explicit DatabaseManager(std::string dbPath = "data/tracker.db");
    ~DatabaseManager();

    // Prevent copying to avoid double close
    DatabaseManager(const DatabaseManager&) = delete;
    DatabaseManager& operator=(const DatabaseManager&) = delete;

    // Allow moving
    DatabaseManager(DatabaseManager&& other) noexcept;
    DatabaseManager& operator=(DatabaseManager&& other) noexcept;

    bool initialize();
    bool executeSchema(const std::string& schemaSqlPath = "data/schema.sql");
    sqlite3* getRawHandle() const;

private:
    std::string dbPath_;
    sqlite3* dbHandle_ = nullptr;
};

} // namespace tracker::db
