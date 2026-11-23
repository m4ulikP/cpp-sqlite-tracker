#pragma once

#include <string>
#include <optional>

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

} // namespace tracker::models
