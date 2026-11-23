#include "services/ItemService.hpp"

#include <algorithm>
#include <cctype>
#include <unordered_set>

namespace tracker::services {

namespace {

std::string trim(const std::string& str) {
    auto start = std::find_if_not(str.begin(), str.end(), [](unsigned char ch) {
        return std::isspace(ch);
    });
    auto end = std::find_if_not(str.rbegin(), str.rend(), [](unsigned char ch) {
        return std::isspace(ch);
    }).base();
    return (start < end) ? std::string(start, end) : std::string();
}

bool isValidDate(const std::string& date) {
    if (date.empty()) return true;
    if (date.length() != 10) return false;
    if (date[4] != '-' || date[7] != '-') return false;
    for (size_t i = 0; i < date.length(); ++i) {
        if (i == 4 || i == 7) continue;
        if (!std::isdigit(static_cast<unsigned char>(date[i]))) return false;
    }
    return true;
}

const std::unordered_set<std::string> VALID_TYPES = {
    "expense", "task"
};

const std::unordered_set<std::string> VALID_CATEGORIES = {
    "Tuition", "Books", "Food", "Housing", "Travel", 
    "Assignment", "Exam", "Project", "Personal", "Other"
};

const std::unordered_set<std::string> VALID_STATUSES = {
    "pending", "in_progress", "completed", "paid", "cancelled"
};

const std::unordered_set<std::string> VALID_PRIORITIES = {
    "low", "medium", "high"
};

} // anonymous namespace

ItemService::ItemService(repository::ItemRepository& repository)
    : repo_(repository) {}

ValidationResult ItemService::validateItem(const models::Item& item) {
    std::string trimmedTitle = trim(item.title);
    if (trimmedTitle.empty()) {
        return ValidationResult::fail("Field 'title' is required and cannot be empty or whitespace.");
    }
    if (trimmedTitle.length() > 150) {
        return ValidationResult::fail("Field 'title' cannot exceed 150 characters.");
    }

    if (VALID_TYPES.find(item.type) == VALID_TYPES.end()) {
        return ValidationResult::fail("Field 'type' must be either 'expense' or 'task'.");
    }

    if (!item.category.empty() && VALID_CATEGORIES.find(item.category) == VALID_CATEGORIES.end()) {
        return ValidationResult::fail("Field 'category' is invalid. Permitted categories: Tuition, Books, Food, Housing, Travel, Assignment, Exam, Project, Personal, Other.");
    }

    if (item.amount < 0.0) {
        return ValidationResult::fail("Field 'amount' must be greater than or equal to 0.0.");
    }

    if (!item.status.empty() && VALID_STATUSES.find(item.status) == VALID_STATUSES.end()) {
        return ValidationResult::fail("Field 'status' must be one of: pending, in_progress, completed, paid, cancelled.");
    }

    if (!item.priority.empty() && VALID_PRIORITIES.find(item.priority) == VALID_PRIORITIES.end()) {
        return ValidationResult::fail("Field 'priority' must be one of: low, medium, high.");
    }

    if (!isValidDate(item.due_date)) {
        return ValidationResult::fail("Field 'due_date' must follow ISO format 'YYYY-MM-DD'.");
    }

    if (item.description.length() > 1000) {
        return ValidationResult::fail("Field 'description' cannot exceed 1000 characters.");
    }

    return ValidationResult::ok();
}

ServiceResult<std::vector<models::Item>> ItemService::listItems(
    const std::optional<std::string>& typeFilter,
    const std::optional<std::string>& statusFilter,
    const std::optional<std::string>& categoryFilter,
    const std::optional<std::string>& search) {

    auto items = repo_.findAll(typeFilter, statusFilter, categoryFilter, search);
    return ServiceResult<std::vector<models::Item>>::ok(std::move(items));
}

ServiceResult<models::Item> ItemService::getItem(int id) {
    if (id <= 0) {
        return ServiceResult<models::Item>::fail("INVALID_INPUT", "ID must be a positive integer.", 400);
    }

    auto item = repo_.findById(id);
    if (!item.has_value()) {
        return ServiceResult<models::Item>::fail("NOT_FOUND", "Item with ID " + std::to_string(id) + " was not found.", 404);
    }

    return ServiceResult<models::Item>::ok(std::move(*item));
}

ServiceResult<models::Item> ItemService::createItem(models::Item item) {
    // Normalization & Defaults
    item.title = trim(item.title);
    if (item.category.empty()) item.category = "Other";
    if (item.status.empty()) item.status = "pending";
    if (item.priority.empty()) item.priority = "medium";

    auto val = validateItem(item);
    if (!val.is_valid) {
        return ServiceResult<models::Item>::fail(val.code, val.message, 400);
    }

    int id = repo_.create(item);
    if (id <= 0) {
        return ServiceResult<models::Item>::fail("DB_ERROR", "Failed to create item in database.", 500);
    }

    auto created = repo_.findById(id);
    if (!created.has_value()) {
        return ServiceResult<models::Item>::fail("DB_ERROR", "Record was created but could not be retrieved.", 500);
    }

    return ServiceResult<models::Item>::ok(std::move(*created));
}

ServiceResult<models::Item> ItemService::updateItem(int id, models::Item item) {
    if (id <= 0) {
        return ServiceResult<models::Item>::fail("INVALID_INPUT", "ID must be a positive integer.", 400);
    }

    auto existing = repo_.findById(id);
    if (!existing.has_value()) {
        return ServiceResult<models::Item>::fail("NOT_FOUND", "Item with ID " + std::to_string(id) + " was not found.", 404);
    }

    item.title = trim(item.title);
    if (item.category.empty()) item.category = existing->category;
    if (item.type.empty()) item.type = existing->type;
    if (item.status.empty()) item.status = existing->status;
    if (item.priority.empty()) item.priority = existing->priority;

    auto val = validateItem(item);
    if (!val.is_valid) {
        return ServiceResult<models::Item>::fail(val.code, val.message, 400);
    }

    bool updated = repo_.update(id, item);
    if (!updated) {
        return ServiceResult<models::Item>::fail("DB_ERROR", "Failed to update item in database.", 500);
    }

    auto result = repo_.findById(id);
    if (!result.has_value()) {
        return ServiceResult<models::Item>::fail("DB_ERROR", "Record was updated but could not be retrieved.", 500);
    }

    return ServiceResult<models::Item>::ok(std::move(*result));
}

ServiceResult<bool> ItemService::deleteItem(int id) {
    if (id <= 0) {
        return ServiceResult<bool>::fail("INVALID_INPUT", "ID must be a positive integer.", 400);
    }

    auto existing = repo_.findById(id);
    if (!existing.has_value()) {
        return ServiceResult<bool>::fail("NOT_FOUND", "Item with ID " + std::to_string(id) + " was not found.", 404);
    }

    bool removed = repo_.remove(id);
    if (!removed) {
        return ServiceResult<bool>::fail("DB_ERROR", "Failed to delete item from database.", 500);
    }

    return ServiceResult<bool>::ok(true);
}

ServiceResult<models::ItemSummaryStats> ItemService::getSummary() {
    auto stats = repo_.getSummaryStats();
    return ServiceResult<models::ItemSummaryStats>::ok(std::move(stats));
}

} // namespace tracker::services
