#pragma once

#include <algorithm>
#include <cmath>
#include <deque>
#include <ob.h>
#include <stream_msg.h>
#include <types.h>

const double EXEC_RATIO =
    0.3; // 30% of the level's quantity is executed, other canceled.

struct L3SmartPriceLevel : L3PriceLevel {
    // confirmed order quantity from l3, but we know for sure that at the moment
    // of the last l2 update, the real qty of this level is l2_qty
    int l2_qty = 0;

    // the last L2 seq_id that was used to update this level
    int last_l2_seq_id = 0;

    int total_unconfirmed_trade_qty = 0;

    // (qty, Trade) pair from trade messages after the last L2 or L3 update
    // qty is the l2_qty at the moment of the Trade message
    std::deque<Trade> unconfirmed_trades;

    void PopUnconfirmedTradesBefore(int seq_id) {
        while (!unconfirmed_trades.empty() &&
               unconfirmed_trades.front().seq_id <= seq_id) {
            total_unconfirmed_trade_qty -= unconfirmed_trades.front().size;
            unconfirmed_trades.pop_front();
        }
    }

    // template <typename Func> void ForEachEstimatedOrders(Func &&func) {
    //     // estimated canceled qty from all the unconfirmed trades
    //     int unconfirmed_canceled_qty =
    //         std::clamp(int(total_unconfirmed_trade_qty * (1 - EXEC_RATIO) / EXEC_RATIO), 0,
    //                    l2_qty - total_unconfirmed_trade_qty - 1);
    //     for (auto &order : orders) {
    //         if (unconfirmed_canceled_qty < order.size) func(order);
    //         else {
    //             unconfirmed_canceled_qty -= order.size;
    //         }
    //     }
    // }
};

struct ObUpdater : private L3Book {
    // This class is responsible for updating the order book with level 2 and
    // level 3 data. It will handle the updates for both L2 and L3 books,
    // ensuring that the order book reflects the latest trades and orders. and
    // triggers OnOrderXXX callbacks.

    // double best_bid() const {
    //     if (last_trade_ask.seq_id > std::max(last_l3_seq_id, last_l2_seq_id)) {
    //         // If the last trade is newer than the last L3 update, return the
    //         // last trade's bid price
    //         return last_trade_bid.price;
    //     } else {
    //         return
    //     }
    // }

    // double best_ask() const {

    // }

    void UpdateL2(const Snapshot &snapshot) {
        if (snapshot.seq_id <= last_l2_seq_id) {
            // Ignore updates that are older than the last L3 update
            return;
        }

        if (snapshot.seq_id > last_trade_ask_id && !snapshot.asks.empty()) {
            RemoveLevel(false, snapshot.asks.front().price);
        }
        if (snapshot.seq_id > last_trade_bid_id && !snapshot.bids.empty()) {
            RemoveLevel(true, snapshot.bids.front().price);
        }

        for (auto &l2_level : snapshot.asks) {
            auto &level = GetOrAddLevel(false, l2_level.price);
            UpdateL2Level(snapshot.seq_id, l2_level.quantity, level);
        }
        for (auto &l2_level : snapshot.bids) {
            auto &level = GetOrAddLevel(true, l2_level.price);
            UpdateL2Level(snapshot.seq_id, l2_level.quantity, level);
        }
    }

    void UpdateL2Level(int seq_id, int l2_qty, L3SmartPriceLevel &level) {
        assert(seq_id > last_l3_seq_id);

        int delta = l2_qty - level.l2_qty;

        if (delta > 0) {
            // OnOrderAdd(delta, level.price, level.is_bid);
        } else {
            if (seq_id <= std::max(last_trade_ask_id, last_trade_bid_id)) {
                // the change between last l2 update and this one is fully due to canceling
                // OnOrderCancel(-delta, level.price, level.is_bid);
            } else {
                // there may be trade between last l2 update and this one,
                // try to guess the cancel and exec qty
                int exec_qty = std::abs(delta) * EXEC_RATIO - level.total_unconfirmed_trade_qty;
                int cancel_qty = std::abs(delta) - exec_qty;
            }
            // NOTE: there maybe additional ADD + CANCEL pairs between these,
            // but there's no point to send them at this point...
        }


        level.last_l2_seq_id = seq_id;
        level.l2_qty = l2_qty;
        level.PopUnconfirmedTradesBefore(seq_id);
        return;
    }

    void UpdateL3(const Level3 &msg) {
        if (msg.seq_id <= last_l3_seq_id) {
            // Ignore updates that are older than the last L3 update
            return;
        }

        last_l3_seq_id = msg.seq_id;
        last_l2_seq_id =
            std::max(last_l2_seq_id, msg.seq_id);
    }

    void ReconcileL3(int seq_id, bool is_bid, L3SmartPriceLevel &level) {
        // if (level.last_l2_seq_id >= seq_id) {
        //     return;
        // }

        if (level.last_l2_seq_id <= seq_id) {
            level.l2_qty = level.qty;
            level.last_l2_seq_id = seq_id;
            level.PopUnconfirmedTradesBefore(seq_id);
            if (level.qty == 0) {
                RemoveLevel(true, level.price);
            }
        }
    }

    void CancelLevels(bool is_bid, double price) {
        // send cancel messages for all orders between the spread
        if (is_bid) {
            for (auto &[p, l] : bids) {
                if (p <= price) break;
                for (auto &order : l.orders) {
                    // OnOrderCancel(order.size, p, true);
                }
            }

            auto it = bids.begin();
            while (it != bids.end() && it->first > price) {
                it = bids.erase(it);
            }

        } else {
            for (auto &[p, l] : asks) {
                if (p >= price) break;
                for (auto &order : l.orders) {
                    // OnOrderCancel(order.size, p, true);
                }
            }

            auto it = asks.begin();
            while (it != asks.end() && it->first < price) {
                it = asks.erase(it);
            }
        }
    }

    void UpdateTrade(const Trade &trade) {
        // the trade is already received by l3 update or guessed by l2 update
        if (trade.seq_id <= last_l3_seq_id ||
            trade.seq_id <= last_l2_seq_id) {
            return;
        }

        CancelLevels(trade.is_buy, trade.price);


        // update the price level's traded list, and trigger the
        // OnOrderExecution callback
        auto &level = GetOrAddLevel(trade.is_buy, trade.price);
        level.unconfirmed_trades.push_back(trade);
        level.total_unconfirmed_trade_qty += trade.size;
        last_trade_seq_id = trade.seq_id;
        (trade.is_buy ? last_trade_bid_id : last_trade_ask_id) = trade.seq_id;
    }

  private:
    int last_l3_seq_id = 0;
    int last_l2_seq_id = 0;
    int last_trade_bid_id = 0, last_trade_ask_id = 0;
};

// level : orders + guessed order - traded orders