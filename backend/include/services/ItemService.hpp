#pragma once

#include "models/Item.hpp"
#include "repository/ItemRepository.hpp"
#include <string>
#include <vector>
#include <optional>

namespace tracker::services {

struct ValidationResult {
    bool is_valid = true;
    std::string error_message;
};

class ItemService {
public:
    explicit ItemService(repository::ItemRepository& repository);

    ValidationResult validateItem(const models::Item& item);
    std::vector<models::Item> listItems(const std::optional<std::string>& typeFilter,
                                        const std::optional<std::string>& statusFilter,
                                        const std::optional<std::string>& categoryFilter,
                                        const std::optional<std::string>& search);
    std::optional<models::Item> getItem(int id);
    std::pair<bool, std::string> createItem(models::Item& item);
    std::pair<bool, std::string> updateItem(int id, const models::Item& item);
    bool deleteItem(int id);
    models::ItemSummaryStats getSummary();

private:
    repository::ItemRepository& repo_;
};

} // namespace tracker::services
