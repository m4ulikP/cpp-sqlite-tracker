#pragma once

#include "models/Item.hpp"
#include <vector>
#include <optional>
#include <string>

struct sqlite3;

namespace tracker::repository {

class ItemRepository {
public:
    explicit ItemRepository(sqlite3* db);

    std::vector<models::Item> findAll(const std::optional<std::string>& typeFilter = std::nullopt,
                                      const std::optional<std::string>& statusFilter = std::nullopt,
                                      const std::optional<std::string>& categoryFilter = std::nullopt,
                                      const std::optional<std::string>& search = std::nullopt);

    std::optional<models::Item> findById(int id);
    int create(const models::Item& item);
    bool update(int id, const models::Item& item);
    bool remove(int id);
    models::ItemSummaryStats getSummaryStats();

private:
    sqlite3* db_;
};

} // namespace tracker::repository
