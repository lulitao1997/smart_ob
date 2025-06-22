#pragma once

#include "stream_msg.h"
#include <cassert>
#include <types.h>
#include <variant>

struct BidComparator {
    bool operator()(const double &lhs, const double &rhs) const {
        return lhs > rhs; // Reverse the normal order
    }
};

struct AskComparator {
    bool operator()(const double &lhs, const double &rhs) const {
        return lhs < rhs;
    }
};

template <typename LevelType, typename Comparator>
using OneSideBook = std::map<double, LevelType, Comparator>;

// struct L2Book {
//     OneSideBook<L2PriceLevel, BidComparator> bids;
//     OneSideBook<L2PriceLevel, AskComparator> asks;
// };

struct L3Book {
    OneSideBook<L3PriceLevel, BidComparator> bids;
    OneSideBook<L3PriceLevel, AskComparator> asks;
    std::unordered_map<int, std::list<Order>::iterator> orderMap;

    // void ProcessMsg(const Level3 &msg) {
    //     // Process the Level 3 message and update the L3 book accordingly
    //     std::visit(
    //         [this](auto &&arg) {
    //             using T = std::decay_t<decltype(arg)>;
    //             if constexpr (std::is_same_v<T, level3::Execute>) {
    //                 Execute(arg.order_id, arg.size > 0, arg.size);
    //             } else if constexpr (std::is_same_v<T, level3::Modify>) {
    //                 Cancel(arg.order_id, arg.is_buy, 0);
    //                 Add(arg.order_id, arg.is_buy, arg.size, arg.price);
    //             } else if constexpr (std::is_same_v<T, level3::Add>) {
    //                 Add(arg.order_id, arg.is_buy, arg.size, arg.price);
    //             } else if constexpr (std::is_same_v<T, level3::Cancel>) {
    //                 Cancel(arg.order_id, arg.is_buy, 0);
    //             } else {
    //                 assert(false && "Unknown message type");
    //             }
    //         },
    //         msg.msg);
    // }

    L3PriceLevel &GetOrAddLevel(bool is_bid, double price) {
        if (is_bid) {
            auto it = bids.find(price);
            if (it == bids.end()) {
                auto &l = bids[price];
                l.price = price;
                return l;
            }
            return it->second;
        } else {
            auto it = asks.find(price);
            if (it == asks.end()) {
                auto &l = asks[price];
                l.price = price;
                return l;
            }
            return it->second;
        }
    }

    void RemoveLevel(bool is_bid, double price) {
        bool erased = false;
        if (is_bid) {
            erased = bids.erase(price);

        } else {
            erased = asks.erase(price);
        }
        assert(erased);
    }

    void Add(int order_id, bool isSell, int size, double price) {
        Order order{order_id, isSell, size, price};
        auto &level = GetOrAddLevel(order.is_buy, order.price);
        level.qty += order.size; // Adjust size
        level.numOrders++;
        level.orders.push_back(order);                  // Add order to the list
        orderMap[order.orderId] = --level.orders.end(); // Store iterator in map
    }

    void Execute(int order_id, bool is_bid, int exec_size) {
        // Execute an order in the L3 book
        auto it = orderMap.find(order_id);

        if (it != orderMap.end()) {
            auto &orderIt = it->second;
            assert(is_bid == orderIt->is_buy);
            auto new_size = orderIt->size - exec_size;
            assert(new_size >= 0);
            Cancel(order_id, is_bid, new_size);
        }
    }

    void Cancel(int order_id, bool is_bid, int new_size) {
        auto it = orderMap.find(order_id);

        if (it != orderMap.end()) {
            auto &orderIt = it->second;
            assert(is_bid == orderIt->is_buy);
            auto price = orderIt->price;
            auto &level = GetOrAddLevel(is_bid, price);

            level.qty += new_size - orderIt->size;
            orderIt->size = new_size;

            if (orderIt->size == 0) {
                // Remove the order if size is zero
                level.numOrders--;
                level.orders.erase(orderIt);
                orderMap.erase(it);

                assert(level.qty >= 0);

                // if (level.qty == 0) {
                //     RemoveLevel(is_bid, price);
                // }

            }
        }
    }
};
