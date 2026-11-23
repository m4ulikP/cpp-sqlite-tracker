#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <cassert>
#include <filesystem>

#include "db/DatabaseManager.hpp"
#include "repository/ItemRepository.hpp"
#include "models/Item.hpp"

// Color output helpers for terminal verification
namespace TermColor {
    const char* RESET   = "\033[0m";
    const char* GREEN   = "\033[32m";
    const char* RED     = "\033[31m";
    const char* YELLOW  = "\033[33m";
    const char* CYAN    = "\033[36m";
    const char* BOLD    = "\033[1m";
}

int totalTests = 0;
int passedTests = 0;

void logTest(const std::string& name, bool condition, const std::string& details = "") {
    totalTests++;
    std::cout << "  [" << (condition ? (std::string(TermColor::GREEN) + "PASS" + TermColor::RESET) 
                                     : (std::string(TermColor::RED) + "FAIL" + TermColor::RESET))
              << "] " << name;
    if (!details.empty()) {
        std::cout << " - " << details;
    }
    std::cout << "\n";
    if (condition) {
        passedTests++;
    }
}

int main() {
    std::cout << TermColor::BOLD << TermColor::CYAN
              << "===================================================================\n"
              << "  Student Expense & Task Tracker - Phase 2 DB & Repository Verification\n"
              << "===================================================================\n"
              << TermColor::RESET << "\n";

    const std::string testDbPath = "data/tracker.db";
    const std::string schemaPath = "data/schema.sql";

    // -------------------------------------------------------------
    // Test Group 1: DatabaseManager Initialization & Schema
    // -------------------------------------------------------------
    std::cout << TermColor::YELLOW << "[1] Verifying DatabaseManager Initialization...\n" << TermColor::RESET;
    tracker::db::DatabaseManager dbManager(testDbPath);
    bool initOk = dbManager.initialize();
    logTest("DatabaseManager::initialize()", initOk, "Opened " + testDbPath + " with WAL & Foreign Keys");

    bool schemaOk = dbManager.executeSchema(schemaPath);
    logTest("DatabaseManager::executeSchema()", schemaOk, "Executed schema from " + schemaPath);

    // Verify idempotency (executing schema again should not crash or duplicate)
    bool idempotentOk = dbManager.executeSchema(schemaPath);
    logTest("DatabaseManager Idempotency", idempotentOk, "Re-running schema executed without error");

    sqlite3* dbHandle = dbManager.getRawHandle();
    logTest("Database Handle Validity", dbHandle != nullptr);

    if (!initOk || !schemaOk || !dbHandle) {
        std::cerr << TermColor::RED << "Fatal: Database initialization failed. Aborting tests.\n" << TermColor::RESET;
        return 1;
    }

    // -------------------------------------------------------------
    // Test Group 2: Repository Construction & Seed Data Retrieval
    // -------------------------------------------------------------
    std::cout << "\n" << TermColor::YELLOW << "[2] Verifying ItemRepository & Seed Records...\n" << TermColor::RESET;
    tracker::repository::ItemRepository repo(dbHandle);

    auto allItems = repo.findAll();
    logTest("ItemRepository::findAll() count >= 6", allItems.size() >= 6, "Retrieved " + std::to_string(allItems.size()) + " items");

    // Verify filter by type: expense
    auto expenseItems = repo.findAll("expense", std::nullopt, std::nullopt, std::nullopt);
    logTest("Filter by type='expense'", !expenseItems.empty() && expenseItems.size() >= 3, 
            "Found " + std::to_string(expenseItems.size()) + " expense records");

    // Verify filter by type: task
    auto taskItems = repo.findAll("task", std::nullopt, std::nullopt, std::nullopt);
    logTest("Filter by type='task'", !taskItems.empty() && taskItems.size() >= 3,
            "Found " + std::to_string(taskItems.size()) + " task records");

    // Verify filter by category
    auto bookItems = repo.findAll(std::nullopt, std::nullopt, "Books", std::nullopt);
    logTest("Filter by category='Books'", !bookItems.empty(), "Found " + std::to_string(bookItems.size()) + " items");

    // Verify search in title/description
    auto searchResults = repo.findAll(std::nullopt, std::nullopt, std::nullopt, "Proposal");
    logTest("Search by keyword 'Proposal'", !searchResults.empty() && searchResults[0].title.find("Proposal") != std::string::npos,
            "Matched: " + (searchResults.empty() ? "None" : searchResults[0].title));

    // Verify findById with existing item
    auto firstItem = repo.findById(allItems[0].id);
    logTest("ItemRepository::findById(existing)", firstItem.has_value() && firstItem->id == allItems[0].id,
            "Fetched ID: " + std::to_string(allItems[0].id));

    // Verify findById with non-existing item
    auto nonExistent = repo.findById(999999);
    logTest("ItemRepository::findById(999999) -> nullopt", !nonExistent.has_value());

    // -------------------------------------------------------------
    // Test Group 3: Create, Update & Delete Lifecycle
    // -------------------------------------------------------------
    std::cout << "\n" << TermColor::YELLOW << "[3] Verifying CRUD Lifecycle (Create, Update, Delete)...\n" << TermColor::RESET;
    tracker::models::Item newItem;
    newItem.title = "Physics Lab Report #2";
    newItem.type = "task";
    newItem.category = "Assignment";
    newItem.amount = 0.0;
    newItem.status = "pending";
    newItem.priority = "high";
    newItem.due_date = "2026-11-29";
    newItem.description = "Optics and diffraction experiment calculations";

    int newId = repo.create(newItem);
    logTest("ItemRepository::create()", newId > 0, "Created item with assigned ID: " + std::to_string(newId));

    // Read back created item
    auto createdItem = repo.findById(newId);
    bool readMatch = createdItem.has_value() && 
                     createdItem->title == newItem.title && 
                     createdItem->category == "Assignment" &&
                     createdItem->priority == "high";
    logTest("Verify Created Item Fields", readMatch, "Title: " + (createdItem ? createdItem->title : "null"));

    // Update the item
    if (createdItem.has_value()) {
        createdItem->title = "Physics Lab Report #2 - Submitted";
        createdItem->status = "completed";
        createdItem->amount = 15.50; // Late printing expense
        createdItem->description = "Printed in campus lab and submitted";

        bool updateOk = repo.update(newId, *createdItem);
        logTest("ItemRepository::update()", updateOk, "Updated record status and amount");

        auto updatedItem = repo.findById(newId);
        bool updateMatch = updatedItem.has_value() && 
                           updatedItem->title == "Physics Lab Report #2 - Submitted" &&
                           updatedItem->status == "completed" &&
                           updatedItem->amount == 15.50;
        logTest("Verify Updated Item Fields", updateMatch, "Status is now: " + (updatedItem ? updatedItem->status : "null"));
    }

    // -------------------------------------------------------------
    // Test Group 4: Summary Analytics & Metric Calculations
    // -------------------------------------------------------------
    std::cout << "\n" << TermColor::YELLOW << "[4] Verifying Dashboard Summary Statistics...\n" << TermColor::RESET;
    auto stats = repo.getSummaryStats();
    bool statsOk = (stats.expenses.total_count > 0) &&
                   (stats.expenses.total_amount > 0.0) &&
                   (stats.tasks.total_count > 0) &&
                   (stats.tasks.completion_percentage >= 0.0 && stats.tasks.completion_percentage <= 100.0);

    logTest("ItemRepository::getSummaryStats()", statsOk,
            "Expenses Total: $" + std::to_string(stats.expenses.total_amount) + 
            ", Task Completion: " + std::to_string(stats.tasks.completion_percentage) + "%");

    // -------------------------------------------------------------
    // Test Group 5: Delete Verification
    // -------------------------------------------------------------
    std::cout << "\n" << TermColor::YELLOW << "[5] Verifying Item Deletion...\n" << TermColor::RESET;
    bool deleteOk = repo.remove(newId);
    logTest("ItemRepository::remove()", deleteOk, "Deleted temporary record ID: " + std::to_string(newId));

    auto afterDelete = repo.findById(newId);
    logTest("Verify Record No Longer Exists", !afterDelete.has_value());

    // -------------------------------------------------------------
    // Test Group 6: SQL Injection & Parameterization Security Scan
    // (Invoking Security Review Principles)
    // -------------------------------------------------------------
    std::cout << "\n" << TermColor::YELLOW << "[6] Security Check: SQL Injection Resilience via Prepared Statements...\n" << TermColor::RESET;
    // Attempt malicious search query
    std::string sqlInjectionPayload = "'; DROP TABLE items; --";
    auto injectionSearch = repo.findAll(std::nullopt, std::nullopt, std::nullopt, sqlInjectionPayload);
    logTest("SQL Injection in Search Filter Handled Safely", injectionSearch.empty(), "Treated as literal string query");

    // Verify items table is intact after injection attempt
    auto tableCheck = repo.findAll();
    logTest("Table Integrity Preserved", !tableCheck.empty(), "Items table exists with " + std::to_string(tableCheck.size()) + " rows");

    // -------------------------------------------------------------
    // Verification Report Summary
    // -------------------------------------------------------------
    std::cout << "\n" << TermColor::BOLD << TermColor::CYAN
              << "===================================================================\n"
              << "  Smoke Test Results: " << passedTests << " / " << totalTests << " passed ("
              << (passedTests == totalTests ? (std::string(TermColor::GREEN) + "ALL PASS") : (std::string(TermColor::RED) + "FAILURES DETECTED"))
              << TermColor::RESET << TermColor::BOLD << TermColor::CYAN << ")\n"
              << "===================================================================\n"
              << TermColor::RESET << "\n";

    return (passedTests == totalTests) ? 0 : 1;
}
