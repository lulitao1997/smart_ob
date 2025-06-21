#pragma once

#include <types.h>
#include <ob.h>
#include <stream_msg.h>

struct ObUpdater {
    // This class is responsible for updating the order book with level 2 and level 3 data.
    // It will handle the updates for both L2 and L3 books, ensuring that the order book reflects the latest trades and orders.
    // and triggers OnOrderXXX callbacks.

    void UpdateL2(const Snapshot &snapshot) {
        // Update the L2 book with the snapshot data
        // This will replace the current bids and asks with the new data
    }
    void UpdateL3(const Level3 &level3) {
        // Update the L3 book with the level 3 data
        // This will handle adds, modifies, and executes of orders
        // and trigger the appropriate callbacks
    }
    void UpdateTrade(const Trade &trade) {
        // Update the order book with the trade data
        // This will handle the trade execution and update the L2 and L3 books accordingly
    }

  private:
    int l3_ground_truth_seq_id = 0;
    int l3_estimated_seq_id = 0;
    L3Book ground_truth, estimated;
    int l2_ground_truth_seq_id = 0;
    L2Book l2_ground_truth;
};