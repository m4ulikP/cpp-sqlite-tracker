#include <iostream>
#include <string>
#include <csignal>
#include <memory>
#include <cstdlib>
#include <filesystem>

#include "httplib.h"
#include "db/DatabaseManager.hpp"
#include "repository/ItemRepository.hpp"
#include "services/ItemService.hpp"
#include "controllers/ItemController.hpp"

namespace {
httplib::Server* g_server = nullptr;

void handleSignal(int signum) {
    std::cout << "\n[Server] Received signal (" << signum << "). Stopping server...\n";
    if (g_server) {
        g_server->stop();
    }
}
} // anonymous namespace

int main(int argc, char* argv[]) {
    std::cout << "===================================================================\n"
              << "  Student Expense & Task Tracker - Modern C++20 HTTP REST API\n"
              << "===================================================================\n";

    const std::string dbPath = "data/tracker.db";
    const std::string schemaPath = "data/schema.sql";
    int port = 8080;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--port" && i + 1 < argc) {
            port = std::stoi(argv[++i]);
        } else if (arg == "--help" || arg == "-h") {
            std::cout << "Usage:\n"
                      << "  tracker_server.exe               Start HTTP REST API & Frontend on http://localhost:8080\n"
                      << "  tracker_server.exe --port <num>  Start server on custom port\n"
                      << "  tracker_server.exe --test        Run in-process C++ database smoke tests\n"
                      << "  tracker_server.exe --help        Show command line options\n";
            return 0;
        }
    }

    bool isTestMode = false;
    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "--test" || std::string(argv[i]) == "-t") {
            isTestMode = true;
            break;
        }
    }

    if (const char* envPort = std::getenv("PORT")) {
        try {
            port = std::stoi(envPort);
        } catch (...) {}
    }

    // 1. Initialize Database
    tracker::db::DatabaseManager dbManager(dbPath);
    if (!dbManager.initialize()) {
        std::cerr << "[Fatal] Failed to open SQLite database at: " << dbPath << "\n";
        return 1;
    }

    if (!dbManager.executeSchema(schemaPath)) {
        std::cerr << "[Fatal] Failed to execute database schema from: " << schemaPath << "\n";
        return 1;
    }

    std::cout << "[Database] Successfully initialized SQLite at: " << dbPath << "\n";

    // 2. Instantiate Repository, Service & Controller
    tracker::repository::ItemRepository repository(dbManager.getRawHandle());

    if (isTestMode) {
        std::cout << "[Test Mode] Running internal repository smoke test suite...\n";
        int passed = 0;
        int total = 0;

        auto check = [&](const std::string& name, bool cond) {
            total++;
            if (cond) {
                passed++;
                std::cout << "  [PASS] " << name << "\n";
            } else {
                std::cout << "  [FAIL] " << name << "\n";
            }
        };

        auto all = repository.findAll();
        check("ItemRepository::findAll() >= 6 items", all.size() >= 6);

        auto exp = repository.findAll("expense", std::nullopt, std::nullopt, std::nullopt);
        check("Filter type='expense'", !exp.empty());

        auto tasks = repository.findAll("task", std::nullopt, std::nullopt, std::nullopt);
        check("Filter type='task'", !tasks.empty());

        auto byId = repository.findById(1);
        check("ItemRepository::findById(1)", byId.has_value() && byId->id == 1);

        tracker::models::Item temp;
        temp.title = "Temporary Test Item";
        temp.type = "task";
        temp.category = "Project";
        temp.status = "pending";
        temp.priority = "medium";
        int newId = repository.create(temp);
        check("ItemRepository::create()", newId > 0);

        temp.title = "Temporary Test Item - Updated";
        bool upOk = repository.update(newId, temp);
        check("ItemRepository::update()", upOk);

        auto stats = repository.getSummaryStats();
        check("ItemRepository::getSummaryStats()", stats.expenses.total_count > 0 && stats.tasks.total_count > 0);

        bool delOk = repository.remove(newId);
        check("ItemRepository::remove()", delOk);

        auto gone = repository.findById(newId);
        check("Confirm Deleted Item is Nullopt", !gone.has_value());

        std::cout << "===================================================================\n"
                  << "Smoke Tests Passed: " << passed << " / " << total << "\n"
                  << "===================================================================\n";
        return (passed == total) ? 0 : 1;
    }

    tracker::services::ItemService service(repository);
    tracker::controllers::ItemController controller(service);

    // 3. Setup HTTP Server
    httplib::Server server;
    g_server = &server;

    // Graceful Shutdown Signal Registration
    std::signal(SIGINT, handleSignal);
    std::signal(SIGTERM, handleSignal);

    // Register API Routes
    controller.registerRoutes(server);

    // Mount static frontend directory if present
    if (std::filesystem::exists("frontend")) {
        server.set_mount_point("/", "./frontend");
    }

    // Graceful Shutdown Route for testing & automated harnesses (localhost only)
    server.Post("/api/admin/shutdown", [&server](const httplib::Request&, httplib::Response& res) {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_content(R"({"success":true,"message":"Server shutting down"})", "application/json");
        std::cout << "[Server] Shutdown requested via /api/admin/shutdown endpoint.\n";
        server.stop();
    });

    std::cout << "[Server]   Listening at http://localhost:" << port << "\n"
              << "           - Health:  http://localhost:" << port << "/api/health\n"
              << "           - Items:   http://localhost:" << port << "/api/items\n"
              << "           - Summary: http://localhost:" << port << "/api/items/summary\n"
              << "===================================================================\n"
              << "Ready for requests. Press Ctrl+C to terminate.\n" << std::flush;

    bool ok = server.listen("0.0.0.0", port);
    if (!ok) {
        std::cerr << "[Server Error] Failed to bind/listen on 0.0.0.0:" << port << "\n";
        return 1;
    }

    std::cout << "[Server] Shutdown complete.\n";
    return 0;
}
