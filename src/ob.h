#pragma once

#include <cassert>
#include <types.h>

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

class L2Book {
  private:
    OneSideBook<L2PriceLevel, BidComparator> bids;
    OneSideBook<L2PriceLevel, AskComparator> asks;

  public:
    // ...
};

class L3Book {
  private:
    OneSideBook<L3PriceLevel, BidComparator> bids;
    OneSideBook<L3PriceLevel, AskComparator> asks;
    std::unordered_map<int, std::list<Order>::iterator> orderMap;

    L3PriceLevel &GetOrAddLevel(bool is_bid, double price) {
        if (is_bid) {
            auto it = bids.find(price);
            if (it == bids.end()) {
                return bids[price];
            }
            return it->second;
        } else {
            auto it = asks.find(price);
            if (it == asks.end()) {
                return asks[price];
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
        Add(order);
    }

    void Add(const Order &order) {
        auto &level = GetOrAddLevel(order.is_buy, order.price);
        level.quantity += order.size; // Adjust size
        level.numOrders++;
        level.orders.push_back(order);                  // Add order to the list
        orderMap[order.orderId] = --level.orders.end(); // Store iterator in map
    }

    void Modify(int order_id, bool is_bid, int new_size, double new_price) {
        // Modify order in the L3 book, new_size=0 means cancel
        auto it = orderMap.find(order_id);

        if (it != orderMap.end()) {
            auto &orderIt = it->second;
            assert(orderIt->is_buy == is_bid);
            auto &level = GetOrAddLevel(is_bid, orderIt->price);

            // Remove the order from the current level
            level.orders.erase(orderIt);
            level.quantity -= orderIt->size; // Adjust size

            if (level.quantity <= 0) {
                RemoveLevel(is_bid, orderIt->price);
            }

            // If new_size is zero, we are effectively canceling the order
            if (new_size > 0) {
                Order modifiedOrder{order_id, orderIt->is_buy, new_size,
                                    new_price};
                Add(modifiedOrder);
            }
        } else {
            // Handle case where order_id does not exist
            // This could be an error or a no-op depending on the requirements
        }
    }

    void Execute(int order_id, bool is_bid, int execute_size) {
        // Execute an order in the L3 book
        auto it = orderMap.find(order_id);

        if (it != orderMap.end()) {
            auto &orderIt = it->second;
            assert(is_bid == orderIt->is_buy);
            auto price = orderIt->price;
            auto &level = GetOrAddLevel(is_bid, orderIt->price);

            // Reduce the size of the order
            level.quantity -= execute_size;
            orderIt->size -= execute_size;

            if (orderIt->size <= 0) {
                // Remove the order if size is zero

                if (orderIt->orderId != level.orders.rbegin()->orderId) {
                    // TODO: cancel the order ranked after this order.
                }

                level.orders.erase(orderIt);
                orderMap.erase(it);

                if (level.quantity <= 0) {
                    RemoveLevel(is_bid, price);
                }
            }
        } else {
            // Handle case where order_id does not exist
            // This could be an error or a no-op depending on the requirements
        }
    }

  public:
    // ...
};

struct OrderBook {
    L2Book l2;
    L3Book l3;
};
