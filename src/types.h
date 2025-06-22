#pragma once

#include <list>
#include <map>
#include <unordered_map>

struct L2PriceLevel {
    double price;
    int quantity;
};

struct Order {
    int orderId;
    bool is_buy;
    int size;
    double price;
};

struct L3PriceLevel {
    double price;
    int qty;
    int numOrders;
    std::list<Order> orders;
};
