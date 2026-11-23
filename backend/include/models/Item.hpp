#pragma once

#include <string>
#include <optional>
#include "nlohmann/json.hpp"

namespace tracker::models {

enum class ItemType {
    Expense,
    Task
};

enum class ItemStatus {
    Pending,
    InProgress,
    Completed,
    Paid,
    Cancelled
};

enum class ItemPriority {
    Low,
    Medium,
    High
};

struct Item {
    int id = 0;
    std::string title;
    std::string type;       // "expense" or "task"
    std::string category;   // "Tuition", "Books", "Food", "Assignment", etc.
    double amount = 0.0;
    std::string status = "pending";
    std::string priority = "medium";
    std::string due_date;   // "YYYY-MM-DD"
    std::string description;
    std::string created_at;
    std::string updated_at;
};

struct ItemSummaryStats {
    struct ExpenseStats {
        int total_count = 0;
        double total_amount = 0.0;
        double paid_amount = 0.0;
        double pending_amount = 0.0;
    } expenses;

    struct TaskStats {
        int total_count = 0;
        int completed_count = 0;
        int in_progress_count = 0;
        int pending_count = 0;
        double completion_percentage = 0.0;
    } tasks;
};

// JSON Serialization helpers
inline void to_json(nlohmann::json& j, const Item& item) {
    j = nlohmann::json{
        {"id", item.id},
        {"title", item.title},
        {"type", item.type},
        {"category", item.category},
        {"amount", item.amount},
        {"status", item.status},
        {"priority", item.priority},
        {"due_date", item.due_date},
        {"description", item.description},
        {"created_at", item.created_at},
        {"updated_at", item.updated_at}
    };
}

inline void from_json(const nlohmann::json& j, Item& item) {
    if (j.contains("id") && !j["id"].is_null()) {
        if (j["id"].is_number()) item.id = j["id"].get<int>();
    }
    if (j.contains("title") && j["title"].is_string()) {
        item.title = j["title"].get<std::string>();
    }
    if (j.contains("type") && j["type"].is_string()) {
        item.type = j["type"].get<std::string>();
    }
    if (j.contains("category") && j["category"].is_string()) {
        item.category = j["category"].get<std::string>();
    }
    if (j.contains("amount") && !j["amount"].is_null()) {
        if (j["amount"].is_number()) item.amount = j["amount"].get<double>();
    }
    if (j.contains("status") && j["status"].is_string()) {
        item.status = j["status"].get<std::string>();
    }
    if (j.contains("priority") && j["priority"].is_string()) {
        item.priority = j["priority"].get<std::string>();
    }
    if (j.contains("due_date") && j["due_date"].is_string()) {
        item.due_date = j["due_date"].get<std::string>();
    }
    if (j.contains("description") && j["description"].is_string()) {
        item.description = j["description"].get<std::string>();
    }
}

inline void to_json(nlohmann::json& j, const ItemSummaryStats& stats) {
    j = nlohmann::json{
        {"expenses", {
            {"total_count", stats.expenses.total_count},
            {"total_amount", stats.expenses.total_amount},
            {"paid_amount", stats.expenses.paid_amount},
            {"pending_amount", stats.expenses.pending_amount}
        }},
        {"tasks", {
            {"total_count", stats.tasks.total_count},
            {"completed_count", stats.tasks.completed_count},
            {"in_progress_count", stats.tasks.in_progress_count},
            {"pending_count", stats.tasks.pending_count},
            {"completion_percentage", stats.tasks.completion_percentage}
        }}
    };
}

} // namespace tracker::models
