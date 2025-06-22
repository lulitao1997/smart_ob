#pragma once

#include <types.h>
#include <variant>
#include <vector>

struct Trade {
    int seq_id;  // Sequence ID of the trade
    bool is_buy;    // is_buy = true means buy order is passive
    double price; // Price of the trade
    int size;     // Size of the trade
};

namespace level3 {
struct Execute {
    int order_id; // Order ID
    int size;
    double price; // Price at which the order was executed
};

struct Cancel {
    int order_id; // Order ID
};

// size=0 means cancel
// Modify = Cancel + Add
struct Modify {
    int order_id; // Order ID
    int new_order_id; // New order ID
    bool is_buy;
    int size;     // New size of the order
    double price; // New price of the order
};

struct Add {
    int order_id; // Order ID
    bool is_buy;
    int size;     // Size of the order
    double price; // Price of the order
};
} // namespace level3

struct Level3 {
    int seq_id;
    std::variant<level3::Execute, level3::Modify, level3::Add> msg;
};


struct Snapshot {
    int seq_id;                     // Sequence ID of the snapshot
    std::vector<L2PriceLevel> bids; // Bids in the snapshot
    std::vector<L2PriceLevel> asks; // Asks in the snapshot
};
