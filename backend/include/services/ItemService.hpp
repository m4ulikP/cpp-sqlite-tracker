#pragma once

#include "models/Item.hpp"
#include "repository/ItemRepository.hpp"
#include <string>
#include <vector>
#include <optional>
#include <utility>

namespace tracker::services {

struct ServiceError {
    std::string code;       // "INVALID_INPUT", "NOT_FOUND", "DB_ERROR"
    std::string message;    // Clear human-readable error description
    int http_status = 400;  // 400, 404, 500
};

template <typename T>
struct ServiceResult {
    bool success = true;
    T data{};
    std::optional<ServiceError> error;

    static ServiceResult<T> ok(T value) {
        ServiceResult<T> r;
        r.success = true;
        r.data = std::move(value);
        return r;
    }

    static ServiceResult<T> fail(std::string code, std::string message, int status = 400) {
        ServiceResult<T> r;
        r.success = false;
        r.error = ServiceError{std::move(code), std::move(message), status};
        return r;
    }
};

struct ValidationResult {
    bool is_valid = true;
    std::string code;
    std::string message;

    static ValidationResult ok() {
        return {true, "", ""};
    }

    static ValidationResult fail(std::string message) {
        return {false, "INVALID_INPUT", std::move(message)};
    }
};

class ItemService {
public:
    explicit ItemService(repository::ItemRepository& repository);

    ValidationResult validateItem(const models::Item& item);

    ServiceResult<std::vector<models::Item>> listItems(
        const std::optional<std::string>& typeFilter,
        const std::optional<std::string>& statusFilter,
        const std::optional<std::string>& categoryFilter,
        const std::optional<std::string>& search);

    ServiceResult<models::Item> getItem(int id);
    ServiceResult<models::Item> createItem(models::Item item);
    ServiceResult<models::Item> updateItem(int id, models::Item item);
    ServiceResult<bool> deleteItem(int id);
    ServiceResult<models::ItemSummaryStats> getSummary();

private:
    repository::ItemRepository& repo_;
};

} // namespace tracker::services
