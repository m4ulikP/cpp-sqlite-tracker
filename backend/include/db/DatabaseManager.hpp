#pragma once

#include <string>
#include <memory>

struct sqlite3;

namespace tracker::db {

class DatabaseManager {
public:
    explicit DatabaseManager(std::string dbPath = "data/tracker.db");
    ~DatabaseManager();

    bool initialize();
    bool executeSchema(const std::string& schemaSqlPath = "data/schema.sql");
    sqlite3* getRawHandle() const;

private:
    std::string dbPath_;
    sqlite3* dbHandle_ = nullptr;
};

} // namespace tracker::db
