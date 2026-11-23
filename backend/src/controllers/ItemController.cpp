#include "controllers/ItemController.hpp"
#include "httplib.h"
#include "nlohmann/json.hpp"

#include <iostream>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace tracker::controllers {

namespace {

std::string getCurrentIsoTimestamp() {
    auto now = std::chrono::system_clock::now();
    std::time_t tt = std::chrono::system_clock::to_time_t(now);
    std::tm gmt{};
#if defined(_WIN32)
    gmtime_s(&gmt, &tt);
#else
    gmtime_r(&tt, &gmt);
#endif
    std::ostringstream ss;
    ss << std::put_time(&gmt, "%Y-%m-%dT%H:%M:%SZ");
    return ss.str();
}

void setCorsHeaders(httplib::Response& res) {
    res.set_header("Access-Control-Allow-Origin", "*");
    res.set_header("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
    res.set_header("Access-Control-Allow-Headers", "Content-Type, Authorization, X-Requested-With");
}

void sendSuccessJson(httplib::Response& res, const nlohmann::json& data, int status = 200) {
    nlohmann::json root = {
        {"success", true},
        {"data", data},
        {"error", nullptr}
    };
    res.status = status;
    res.set_content(root.dump(2), "application/json");
}

void sendErrorJson(httplib::Response& res, const std::string& code, const std::string& message, int status = 400) {
    nlohmann::json root = {
        {"success", false},
        {"data", nullptr},
        {"error", {
            {"code", code},
            {"message", message}
        }}
    };
    res.status = status;
    res.set_content(root.dump(2), "application/json");
}

} // anonymous namespace

ItemController::ItemController(services::ItemService& service)
    : service_(service) {}

void ItemController::registerRoutes(httplib::Server& server) {

    // 0. Global CORS Pre-Routing Handler
    server.set_pre_routing_handler([](const httplib::Request& req, httplib::Response& res) {
        setCorsHeaders(res);
        if (req.method == "OPTIONS") {
            res.status = 204;
            return httplib::Server::HandlerResponse::Handled;
        }
        return httplib::Server::HandlerResponse::Unhandled;
    });

    server.Options(R"(.*)", [](const httplib::Request&, httplib::Response& res) {
        setCorsHeaders(res);
        res.status = 204;
    });

    // 1. Health Check: GET /api/health
    server.Get("/api/health", [](const httplib::Request&, httplib::Response& res) {
        nlohmann::json healthData = {
            {"status", "healthy"},
            {"database", "connected"},
            {"version", "1.0.0"},
            {"timestamp", getCurrentIsoTimestamp()}
        };
        sendSuccessJson(res, healthData, 200);
    });

    // 2. Summary Statistics: GET /api/items/summary
    // (Registered before regex /api/items/(\d+) to avoid matching 'summary' as ID)
    server.Get("/api/items/summary", [this](const httplib::Request&, httplib::Response& res) {
        auto result = service_.getSummary();
        if (!result.success) {
            sendErrorJson(res, result.error->code, result.error->message, result.error->http_status);
            return;
        }
        nlohmann::json j = result.data;
        sendSuccessJson(res, j, 200);
    });

    // 3. List Items: GET /api/items (with query filters)
    server.Get("/api/items", [this](const httplib::Request& req, httplib::Response& res) {
        std::optional<std::string> typeFilter = req.has_param("type") ? std::optional<std::string>(req.get_param_value("type")) : std::nullopt;
        std::optional<std::string> statusFilter = req.has_param("status") ? std::optional<std::string>(req.get_param_value("status")) : std::nullopt;
        std::optional<std::string> categoryFilter = req.has_param("category") ? std::optional<std::string>(req.get_param_value("category")) : std::nullopt;
        std::optional<std::string> search = req.has_param("search") ? std::optional<std::string>(req.get_param_value("search")) : std::nullopt;

        auto result = service_.listItems(typeFilter, statusFilter, categoryFilter, search);
        if (!result.success) {
            sendErrorJson(res, result.error->code, result.error->message, result.error->http_status);
            return;
        }

        nlohmann::json arr = nlohmann::json::array();
        for (const auto& item : result.data) {
            arr.push_back(item);
        }
        sendSuccessJson(res, arr, 200);
    });

    // 4. Get Item by ID: GET /api/items/(\d+)
    server.Get(R"(/api/items/(\d+))", [this](const httplib::Request& req, httplib::Response& res) {
        try {
            int id = std::stoi(req.matches[1].str());
            auto result = service_.getItem(id);
            if (!result.success) {
                sendErrorJson(res, result.error->code, result.error->message, result.error->http_status);
                return;
            }
            nlohmann::json j = result.data;
            sendSuccessJson(res, j, 200);
        } catch (const std::exception&) {
            sendErrorJson(res, "INVALID_INPUT", "Invalid item ID format.", 400);
        }
    });

    // 5. Create Item: POST /api/items
    server.Post("/api/items", [this](const httplib::Request& req, httplib::Response& res) {
        if (req.body.empty()) {
            sendErrorJson(res, "INVALID_JSON", "Request body cannot be empty.", 400);
            return;
        }

        nlohmann::json bodyJson;
        try {
            bodyJson = nlohmann::json::parse(req.body);
        } catch (const nlohmann::json::parse_error& e) {
            sendErrorJson(res, "INVALID_JSON", std::string("Malformed JSON payload: ") + e.what(), 400);
            return;
        }

        if (!bodyJson.is_object()) {
            sendErrorJson(res, "INVALID_JSON", "Request payload must be a JSON object.", 400);
            return;
        }

        models::Item item{};
        try {
            item = bodyJson.get<models::Item>();
        } catch (const std::exception& e) {
            sendErrorJson(res, "INVALID_INPUT", std::string("Field parsing error: ") + e.what(), 400);
            return;
        }

        auto result = service_.createItem(item);
        if (!result.success) {
            sendErrorJson(res, result.error->code, result.error->message, result.error->http_status);
            return;
        }

        nlohmann::json j = result.data;
        sendSuccessJson(res, j, 201); // 201 Created
    });

    // 6. Update Item: PUT /api/items/(\d+)
    server.Put(R"(/api/items/(\d+))", [this](const httplib::Request& req, httplib::Response& res) {
        int id = 0;
        try {
            id = std::stoi(req.matches[1].str());
        } catch (const std::exception&) {
            sendErrorJson(res, "INVALID_INPUT", "Invalid item ID format.", 400);
            return;
        }

        if (req.body.empty()) {
            sendErrorJson(res, "INVALID_JSON", "Request body cannot be empty.", 400);
            return;
        }

        nlohmann::json bodyJson;
        try {
            bodyJson = nlohmann::json::parse(req.body);
        } catch (const nlohmann::json::parse_error& e) {
            sendErrorJson(res, "INVALID_JSON", std::string("Malformed JSON payload: ") + e.what(), 400);
            return;
        }

        if (!bodyJson.is_object()) {
            sendErrorJson(res, "INVALID_JSON", "Request payload must be a JSON object.", 400);
            return;
        }

        models::Item item{};
        try {
            item = bodyJson.get<models::Item>();
        } catch (const std::exception& e) {
            sendErrorJson(res, "INVALID_INPUT", std::string("Field parsing error: ") + e.what(), 400);
            return;
        }

        auto result = service_.updateItem(id, item);
        if (!result.success) {
            sendErrorJson(res, result.error->code, result.error->message, result.error->http_status);
            return;
        }

        nlohmann::json j = result.data;
        sendSuccessJson(res, j, 200);
    });

    // 7. Delete Item: DELETE /api/items/(\d+)
    server.Delete(R"(/api/items/(\d+))", [this](const httplib::Request& req, httplib::Response& res) {
        int id = 0;
        try {
            id = std::stoi(req.matches[1].str());
        } catch (const std::exception&) {
            sendErrorJson(res, "INVALID_INPUT", "Invalid item ID format.", 400);
            return;
        }

        auto result = service_.deleteItem(id);
        if (!result.success) {
            sendErrorJson(res, result.error->code, result.error->message, result.error->http_status);
            return;
        }

        nlohmann::json data = {
            {"id", id},
            {"deleted", true}
        };
        sendSuccessJson(res, data, 200);
    });

    // Catch invalid non-numeric item subpaths e.g. /api/items/not-a-number
    server.Get(R"(/api/items/([^/]+))", [](const httplib::Request&, httplib::Response& res) {
        sendErrorJson(res, "INVALID_INPUT", "Item ID must be a positive integer.", 400);
    });
}

} // namespace tracker::controllers
