#pragma once

#include <list>
#include <map>
#include <unordered_map>

struct L2PriceLevel {
    double price;
    int qty;
};

struct Order {
    int orderId;
    bool is_buy;
    int size;
    double price;
};

struct L3PriceLevel {
    bool is_bid;
    double price;
    int qty;
    int numOrders;
    // orders at the back of the list are the most recent
    std::list<Order> orders;
};
