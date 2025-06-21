#pragma once
#include <algorithm>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

#include "types.h"

inline std::vector<Trade> read_from_csv(std::istream &input) {
    std::vector<Trade> trades;

    // TIMESTAMP,SYMBOL,BUY_OR_SELL,PRICE,QUANTITY
    // 101,TFS,B,11.00,15
    // 102,TFS,B,12.50,15
    // 103,TFS,S,13.00,20
    // 104,TFS,S,12.75,10

    std::string line;
    std::getline(input, line);  // skip header

    while (std::getline(input, line)) {
        std::istringstream ss(line);
        Trade trade;
        std::string ts, buy_sell, price, size;

        std::getline(ss, ts, ',');
        std::getline(ss, trade.symbol, ',');
        std::getline(ss, buy_sell, ',');
        std::getline(ss, price, ',');
        std::getline(ss, size);

        trade.ts = std::stoi(ts);
        trade.side = (buy_sell == "S");
        trade.price = std::stod(price);
        trade.size = std::stoi(size);

        trades.push_back(trade);
    }

    return trades;
}

inline std::string pnl_to_string(const PnlRecord &record) {
    // e.g. 104,TFS,2.50
    std::stringstream ss;
    ss << record.ts << "," << record.symbol << "," << std::fixed << std::setprecision(2) << record.pnl;
    return ss.str();
}