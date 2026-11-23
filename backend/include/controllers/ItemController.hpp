#pragma once

#include "services/ItemService.hpp"

namespace httplib {
    class Server;
}

namespace tracker::controllers {

class ItemController {
public:
    explicit ItemController(services::ItemService& service);

    void registerRoutes(httplib::Server& server);

private:
    services::ItemService& service_;
};

} // namespace tracker::controllers
