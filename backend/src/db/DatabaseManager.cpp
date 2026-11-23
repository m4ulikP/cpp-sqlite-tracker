#include "db/DatabaseManager.hpp"
#include "sqlite3.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <utility>

namespace tracker::db {

DatabaseManager::DatabaseManager(std::string dbPath)
    : dbPath_(std::move(dbPath)), dbHandle_(nullptr) {}

DatabaseManager::~DatabaseManager() {
    if (dbHandle_) {
        sqlite3_close_v2(dbHandle_);
        dbHandle_ = nullptr;
    }
}

DatabaseManager::DatabaseManager(DatabaseManager&& other) noexcept
    : dbPath_(std::move(other.dbPath_)), dbHandle_(other.dbHandle_) {
    other.dbHandle_ = nullptr;
}

DatabaseManager& DatabaseManager::operator=(DatabaseManager&& other) noexcept {
    if (this != &other) {
        if (dbHandle_) {
            sqlite3_close_v2(dbHandle_);
        }
        dbPath_ = std::move(other.dbPath_);
        dbHandle_ = other.dbHandle_;
        other.dbHandle_ = nullptr;
    }
    return *this;
}

bool DatabaseManager::initialize() {
    try {
        std::filesystem::path p(dbPath_);
        if (p.has_parent_path()) {
            std::filesystem::create_directories(p.parent_path());
        }
    } catch (const std::exception& e) {
        std::cerr << "[DatabaseManager Error] Failed to create directories for db path '" 
                  << dbPath_ << "': " << e.what() << "\n";
        return false;
    }

    int rc = sqlite3_open_v2(
        dbPath_.c_str(),
        &dbHandle_,
        SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE,
        nullptr
    );

    if (rc != SQLITE_OK) {
        std::cerr << "[DatabaseManager Error] Failed to open SQLite database at '" 
                  << dbPath_ << "': " << (dbHandle_ ? sqlite3_errmsg(dbHandle_) : "Unknown error") << "\n";
        if (dbHandle_) {
            sqlite3_close_v2(dbHandle_);
            dbHandle_ = nullptr;
        }
        return false;
    }

    // Enable Foreign Key enforcement
    char* errMsg = nullptr;
    rc = sqlite3_exec(dbHandle_, "PRAGMA foreign_keys = ON;", nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        std::cerr << "[DatabaseManager Warning] Failed to enable PRAGMA foreign_keys: " 
                  << (errMsg ? errMsg : "Unknown error") << "\n";
        sqlite3_free(errMsg);
    }

    // Enable Write-Ahead Logging (WAL) for better concurrency and performance
    errMsg = nullptr;
    rc = sqlite3_exec(dbHandle_, "PRAGMA journal_mode = WAL;", nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        std::cerr << "[DatabaseManager Warning] Failed to enable PRAGMA journal_mode = WAL: " 
                  << (errMsg ? errMsg : "Unknown error") << "\n";
        sqlite3_free(errMsg);
    }

    return true;
}

bool DatabaseManager::executeSchema(const std::string& schemaSqlPath) {
    if (!dbHandle_) {
        std::cerr << "[DatabaseManager Error] Cannot execute schema: database handle is null.\n";
        return false;
    }

    if (!std::filesystem::exists(schemaSqlPath)) {
        std::cerr << "[DatabaseManager Error] Schema file not found at '" << schemaSqlPath << "'.\n";
        return false;
    }

    std::ifstream file(schemaSqlPath);
    if (!file.is_open()) {
        std::cerr << "[DatabaseManager Error] Unable to open schema file: " << schemaSqlPath << "\n";
        return false;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string sql = buffer.str();

    char* errMsg = nullptr;
    int rc = sqlite3_exec(dbHandle_, sql.c_str(), nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        std::cerr << "[DatabaseManager Error] Failed to execute schema SQL: " 
                  << (errMsg ? errMsg : "Unknown error") << "\n";
        sqlite3_free(errMsg);
        return false;
    }

    return true;
}

sqlite3* DatabaseManager::getRawHandle() const {
    return dbHandle_;
}

} // namespace tracker::db
