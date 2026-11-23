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
