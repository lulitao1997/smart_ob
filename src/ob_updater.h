#pragma once

#include <algorithm>
#include <callback.h>
#include <cmath>
#include <deque>
#include <limits>
#include <ob.h>
#include <stream_msg.h>
#include <types.h>

const double EXEC_RATIO =
    0.3; // 30% of the level's quantity is executed, other canceled.

struct L3SmartPriceLevel : L3PriceLevel {
    // confirmed order quantity from l2 snapshot
    int l2_qty = 0;

    int total_unconfirmed_trade_qty = 0;

    // trade messages after the last L2 or L3 update
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
    //         std::clamp(int(total_unconfirmed_trade_qty * (1 - EXEC_RATIO) /
    //         EXEC_RATIO), 0,
    //                    l2_qty - total_unconfirmed_trade_qty - 1);
    //     for (auto &order : orders) {
    //         if (unconfirmed_canceled_qty < order.size) func(order);
    //         else {
    //             unconfirmed_canceled_qty -= order.size;
    //         }
    //     }
    // }
};

struct SmartL3Book : private L3BookImpl<L3SmartPriceLevel> {
    SmartL3Book(SmartObCallback *callback) : callback(callback) {}

    void UpdateL2(const Snapshot &snapshot) {
        if (snapshot.seq_id <= last_l2_seq_id) {
            // Ignore updates that are older than the last L3 update
            return;
        }

        if (snapshot.seq_id > last_trade_ask_id) {
            auto best_ask = snapshot.asks.empty()
                                ? std::numeric_limits<double>::max()
                                : snapshot.asks.front().price;
            RemoveLevel(false, best_ask);
            last_l2_best_ask = best_ask;
        }
        if (snapshot.seq_id > last_trade_bid_id && !snapshot.bids.empty()) {
            auto best_bid =
                snapshot.bids.empty() ? 0 : snapshot.bids.front().price;
            RemoveLevel(true, best_bid);
            last_l2_best_bid = best_bid;
        }

        for (auto &l2_level : snapshot.asks) {
            auto &level = GetOrAddLevel(false, l2_level.price);
            UpdateL2Level(snapshot.seq_id, l2_level.quantity, level);
        }
        for (auto &l2_level : snapshot.bids) {
            auto &level = GetOrAddLevel(true, l2_level.price);
            UpdateL2Level(snapshot.seq_id, l2_level.quantity, level);
        }

        last_l2_seq_id = snapshot.seq_id;
    }

    void UpdateL2Level(int seq_id, int l2_qty, L3SmartPriceLevel &level) {
        assert(seq_id > last_l3_seq_id);

        int delta = l2_qty - level.l2_qty;

        if (delta > 0) {
            // OnOrderAdd(delta, level.price, level.is_bid);
            callback->onOrderAdd(
                *this, OrderInfo{0, level.is_bid, delta, level.price});

        } else {
            if (seq_id <= std::max(last_trade_ask_id, last_trade_bid_id)) {
                // the change between last l2 update and this one is fully due
                // to canceling OnOrderCancel(-delta, level.price,
                // level.is_bid);
            } else {
                // there may be trade between last l2 update and this one,
                // try to guess the cancel and exec qty
                int exec_qty = std::abs(delta) * EXEC_RATIO -
                               level.total_unconfirmed_trade_qty;
                int cancel_qty = std::abs(delta) - exec_qty;
                callback->onOrderExecution(
                    *this, OrderInfo{0, level.is_bid, exec_qty, level.price});
                callback->onOrderCancel(
                    *this, OrderInfo{0, level.is_bid, cancel_qty, level.price});
            }
            // NOTE: there maybe additional ADD + CANCEL pairs between these,
            // but there's no point to send them at this point...
        }

        level.l2_qty = l2_qty;
        level.PopUnconfirmedTradesBefore(seq_id);
        return;
    }

    void UpdateL3(const Level3 &msg) {

        ProcessMsg(msg, [this, seq_id = msg.seq_id](auto &level) {
            ReconcileL3(seq_id, level);
        });

        if (msg.seq_id < last_l2_seq_id) {
            CancelLevels(true, last_l2_best_bid, false);
            CancelLevels(false, last_l2_best_ask, false);
        }
        if (msg.seq_id <= last_trade_bid_id) {
            CancelLevels(true, last_l2_best_bid, false);
        }
        if (msg.seq_id <= last_trade_ask_id) {
            CancelLevels(false, last_l2_best_ask, false);
        }

        if (msg.seq_id >
            std::max({last_l2_seq_id, last_trade_bid_id, last_trade_ask_id})) {

            // pass through the level3 msg to the callback
            std::visit(
                [this](auto &&arg) {
                    using T = std::decay_t<decltype(arg)>;
                    if constexpr (std::is_same_v<T, level3::Execute>) {
                        callback->onOrderExecution(
                            *this, OrderInfo{arg.order_id, arg.is_buy, arg.size,
                                             arg.price});
                    } else if constexpr (std::is_same_v<T, level3::Modify>) {
                        callback->onOrderModify(
                            *this,
                            OrderInfo{arg.order_id, arg.is_buy, arg.size, arg.price});
                    } else if constexpr (std::is_same_v<T, level3::Add>) {
                        callback->onOrderAdd(*this,
                                             OrderInfo{arg.order_id, arg.is_buy,
                                                       arg.size, arg.price});
                    } else if constexpr (std::is_same_v<T, level3::Cancel>) {
                        callback->onOrderCancel(
                            *this,
                            OrderInfo{arg.order_id, arg.is_buy, 0, arg.price});
                    } else {
                        assert(false && "Unknown message type");
                    }
                },
                msg.msg);
        }

        last_l3_seq_id = msg.seq_id;
        last_l2_seq_id = std::max(last_l2_seq_id, msg.seq_id);
    }

    void ReconcileL3(int seq_id, L3SmartPriceLevel &level) {
        if (last_l2_seq_id <= seq_id) {
            level.l2_qty = level.qty;
            last_l2_seq_id = seq_id;
            level.PopUnconfirmedTradesBefore(seq_id);
            if (level.qty == 0) {
                RemoveLevel(level.is_bid, level.price);
            }
        }
    }

    void CancelLevels(bool is_bid, double price, bool send_cancel) {
        // send cancel messages for all orders between the spread
        if (is_bid) {
            if (send_cancel)
                for (auto &[p, l] : bids) {
                    if (p <= price)
                        break;
                    for (auto &order : l.orders) {
                        // OnOrderCancel(order.size, p, true);
                    }
                }

            auto it = bids.begin();
            while (it != bids.end() && it->first > price) {
                it = bids.erase(it);
            }

        } else {
            if (send_cancel)
                for (auto &[p, l] : asks) {
                    if (p >= price)
                        break;
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
        // the trade is already received by l3 update or guessed by l2 update,
        // skip
        if (trade.seq_id <= last_l3_seq_id || trade.seq_id <= last_l2_seq_id) {
            return;
        }

        CancelLevels(trade.is_buy, trade.price, true);

        // update the price level's traded list, and trigger the
        // OnOrderExecution callback
        auto &level = GetOrAddLevel(trade.is_buy, trade.price);
        level.unconfirmed_trades.push_back(trade);
        level.total_unconfirmed_trade_qty += trade.size;
        (trade.is_buy ? last_trade_bid_id : last_trade_ask_id) = trade.seq_id;
    }

  private:
    SmartObCallback *callback;

    int last_l3_seq_id = 0;
    int last_l2_seq_id = 0;
    double last_l2_best_bid = 0.0,
           last_l2_best_ask = std::numeric_limits<double>::max();
    int last_trade_bid_id = 0, last_trade_ask_id = 0;
};

// level : orders + guessed order - traded orders