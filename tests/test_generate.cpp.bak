#include <gtest/gtest.h>

#include <random>
#include <string>
#include <unordered_map>
#include <vector>

#include "pnl_calculator.h"

std::vector<Trade> generate_flat_trades() {
    std::vector<Trade> trades;
    std::vector<std::string> symbols = {"AAPL", "GOOG", "TSLA"};
    int ts = 100;
    std::mt19937 rng(42);  // fixed seed for reproducibility
    std::uniform_int_distribution<int> dist_size(1, 50);
    std::uniform_int_distribution<int> dist_symbol(0, symbols.size() - 1);
    std::uniform_int_distribution<int> dist_side(0, 1);

    // Track net position for each symbol
    std::unordered_map<std::string, int> pos;
    int n = 100;
    for (int i = 0; i < n; ++i) {
        std::string sym = symbols[dist_symbol(rng)];
        bool side = dist_side(rng);
        int size = dist_size(rng);
        double price = 150.0 + dist_size(rng);  // random-ish price

        trades.push_back({ts++, sym, side, price, size});
        // Update net position
        pos[sym] += side ? -size : size;
    }

    // Ensure all positions are flat
    for (const auto &sym : symbols) {
        if (pos[sym] != 0) {
            // Add a final trade to flatten
            bool side = pos[sym] > 0 ? 1 : 0;
            int size = std::abs(pos[sym]);
            double price = 150.0 + dist_size(rng);
            trades.push_back({ts++, sym, side, price, size});
            pos[sym] += side ? -size : size;
        }
    }

    return trades;
}

TEST(Test, TestPnlOnGeneratedTrades) {
    auto trades = generate_flat_trades();
    std::unordered_map<std::string, int> pos_map;

    for (const auto &trade : trades) {
        pos_map[trade.symbol] += trade.side ? -trade.size : trade.size;
    }

    for (const auto &[symbol, pos] : pos_map) {
        ASSERT_EQ(pos, 0);
    }

    // Use FIFO
    auto pnl_fifo = calculate_pnl(trades, false);
    // Use LIFO
    auto pnl_lifo = calculate_pnl(trades, true);

    // Check that both methods yield the same number of records
    ASSERT_EQ(pnl_fifo.size(), pnl_lifo.size());

    // Check that the PnL records are consistent
    std::unordered_map<std::string, double> pnl_map_fifo, pnl_map_lifo;
    for (size_t i = 0; i < pnl_fifo.size(); ++i) {
        ASSERT_EQ(pnl_fifo[i].ts, pnl_lifo[i].ts);
        ASSERT_EQ(pnl_fifo[i].symbol, pnl_lifo[i].symbol);
    }

    for (auto &[symbol, pnl] : pnl_map_fifo) {
        ASSERT_NEAR(pnl, pnl_map_lifo[symbol], 1e-6);
    }
}