#pragma once

#include <algorithm>
#include <callback.h>
#include <cmath>
#include <deque>
#include <limits>
#include <ob.h>
#include <stream_msg.h>
#include <string>
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

    std::vector<Order> GetOrders() const {
        std::vector<Order> orders;
        // cancel orders at the front
        int should_cancel_qty = std::max(0, qty - l2_qty);
        for (const auto &order : this->orders) {
            if (order.size > should_cancel_qty) {
                auto order_cpy = order;
                order_cpy.size -= should_cancel_qty;
                orders.push_back(order_cpy);
                should_cancel_qty = 0;
            } else {
                should_cancel_qty -= order.size;
            }
        }
        if (qty > l2_qty) {
            int guessed_qty = qty - l2_qty;
            Order guessed_order{0, is_bid, guessed_qty, price};
            orders.push_back(guessed_order);
        }

        // remove the trades from behind
        auto trade_remaining_qty =
            total_unconfirmed_trade_qty;
        while (!orders.empty() && trade_remaining_qty > 0) {
            if (orders.back().size <= total_unconfirmed_trade_qty) {
                trade_remaining_qty -= orders.back().size;
                orders.pop_back();
            } else {
                orders.back().size -= trade_remaining_qty;
                break; // no more trades to remove
            }
        }
        return orders;
    }

    std::string ToString() const {
        std::string result = std::to_string(price);
        result += ":[";
        for (const auto &order : orders) {
            result += std::to_string(order.size) + '@' + std::to_string(order.orderId);
        }
        result += "]";
        return result;
    }

    void DebugCheck() const {
        int l3_total_qty = 0;
        for (const auto &order : orders) {
            assert(order.price == price);
            assert(order.is_buy == is_bid);
            l3_total_qty += order.size;
        }
        assert(l3_total_qty == qty);

        int guessed_total_qty = 0;
        auto guess_orders = GetOrders();
        for (const auto &order : guess_orders) {
            assert(order.price == price);
            assert(order.is_buy == is_bid);
            guessed_total_qty += order.size;
        }
        assert(l2_qty - total_unconfirmed_trade_qty < 0 ||
               l2_qty - total_unconfirmed_trade_qty == guessed_total_qty);
    }
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
                            *this, OrderInfo{arg.order_id, arg.is_buy, arg.size,
                                             arg.price});
                    } else if constexpr (std::is_same_v<T, level3::Add>) {
                        callback->onOrderAdd(*this,
                                             OrderInfo{arg.order_id, arg.is_buy,
                                                       arg.size, arg.price});
                    } else if constexpr (std::is_same_v<T, level3::Cancel>) {
                        callback->onOrderCancel(
                            *this,
                            OrderInfo{arg.order_id, arg.is_buy, 0, 0});
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
                        callback->onOrderCancel(
                            *this, OrderInfo{order.orderId, order.is_buy,
                                             order.size, p});
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
                        callback->onOrderCancel(
                            *this, OrderInfo{order.orderId, order.is_buy,
                                             order.size, p});
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

    std::string ToString() const {
        std::string result;
        result += "BID:\n";
        for (const auto &[price, level] : bids) {
            result += level.ToString() + "\n";
        }
        result += "ASK:\n";
        for (const auto &[price, level] : asks) {
            result += level.ToString() + "\n";
        }
        return result;
    }

    void DebugCheck() const {
        for (const auto &[price, level] : bids) {
            level.DebugCheck();
        }
        for (const auto &[price, level] : asks) {
            level.DebugCheck();
        }
        assert(!bids.empty() || !asks.empty() || bids.begin()->first < asks.begin()->first);
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