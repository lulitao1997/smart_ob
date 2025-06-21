#include <gtest/gtest.h>
#include <pnl_calculator.h>
#include <reader.h>

#include "types.h"

TEST(Test, ReadFromCSV) {
    std::istringstream input(
        "TIMESTAMP,SYMBOL,BUY_OR_SELL,PRICE,QUANTITY\n"
        "101,TFS,B,11.00,15\n"
        "102,TFS,B,12.50,15\n"
        "103,TFS,S,13.00,20\n"
        "104,TFS,S,12.75,10\n"
        "105,TFD,B,10.75,10\n");

    auto trades = read_from_csv(input);

    std::vector<Trade> expected = {{101, "TFS", false, 11.00, 15},
                                   {102, "TFS", false, 12.50, 15},
                                   {103, "TFS", true, 13.00, 20},
                                   {104, "TFS", true, 12.75, 10},
                                   {105, "TFD", false, 10.75, 10}};

    ASSERT_EQ(trades, expected);
}

TEST(TEST, CalculatePnl1) {
    auto pnl_records = calculate_pnl({}, false);
    ASSERT_EQ(pnl_records, std::vector<PnlRecord>{});

    pnl_records = calculate_pnl({}, true);
    ASSERT_EQ(pnl_records, std::vector<PnlRecord>{});
}

TEST(Test, CalculatePnl2) {
    std::vector<Trade> trades = {
        {101, "A", false, 11.00, 15},
        {102, "B", false, 12.50, 15},
        {103, "A", true, 13.00, 15},
        {104, "B", true, 12.75, 15},
    };

    auto pnl_records = calculate_pnl(trades, false);
    auto expected_pnl = std::vector<PnlRecord>{{103, "A", 15 * (13 - 11.00)}, {104, "B", 15 * (12.75 - 12.50)}};
    ASSERT_EQ(pnl_records, expected_pnl);

    pnl_records = calculate_pnl(trades, true);
    ASSERT_EQ(pnl_records, expected_pnl);
}

TEST(Test, CalculatePnl) {
    std::vector<Trade> trades = {
        {101, "TFS", false, 11.00, 15},
        {102, "TFS", false, 12.50, 15},
        {103, "TFS", true, 13.00, 20},
        {104, "TFS", true, 12.75, 10},
    };

    auto pnl_records = calculate_pnl(trades, false);
    auto expected_pnl = std::vector<PnlRecord>{{103, "TFS", 15 * (13.00 - 11.00) + 5 * (13.00 - 12.50)},
                                               {104, "TFS", 10 * (12.75 - 12.50)}};
    ASSERT_EQ(pnl_records, expected_pnl);

    pnl_records = calculate_pnl(trades, true);
    expected_pnl = std::vector<PnlRecord>{{103, "TFS", 15 * (13.00 - 12.50) + 5 * (13.00 - 11.00)},
                                          {104, "TFS", 10 * (12.75 - 11.00)}};
    ASSERT_EQ(pnl_records, expected_pnl);
}

TEST(Test, CalculatePnlOpposite) {
    std::vector<Trade> trades = {
        {101, "TFS", true, 11.00, 15},
        {102, "TFS", true, 12.50, 15},
        {103, "TFS", false, 13.00, 20},
        {104, "TFS", false, 12.75, 10},
    };

    auto pnl_records = calculate_pnl(trades, false);
    auto expected_pnl = std::vector<PnlRecord>{{103, "TFS", -15 * (13.00 - 11.00) - 5 * (13.00 - 12.50)},
                                               {104, "TFS", -10 * (12.75 - 12.50)}};
    ASSERT_EQ(pnl_records, expected_pnl);

    pnl_records = calculate_pnl(trades, true);
    expected_pnl = std::vector<PnlRecord>{{103, "TFS", -15 * (13.00 - 12.50) - 5 * (13.00 - 11.00)},
                                          {104, "TFS", -10 * (12.75 - 11.00)}};
    ASSERT_EQ(pnl_records, expected_pnl);
}

TEST(Test, CalculatePnlOverSell) {
    std::vector<Trade> trades = {
        {101, "TFS", true, 11.00, 15},
        {102, "TFS", true, 12.50, 15},
        {103, "TFS", false, 13.00, 35},
        {104, "TFS", true, 12.75, 5},
    };

    auto pnl_records = calculate_pnl(trades, false);
    auto expected_pnl =
        std::vector<PnlRecord>{{103, "TFS", -13.00 * 35 + 11.00 * 15 + 12.50 * 15}, {104, "TFS", 5 * (12.75 - 13.00)}};
    for (size_t i = 0; i < pnl_records.size(); ++i) {
        ASSERT_EQ(pnl_records[i].ts, expected_pnl[i].ts);
        ASSERT_EQ(pnl_records[i].symbol, expected_pnl[i].symbol);
        ASSERT_DOUBLE_EQ(pnl_records[i].pnl, expected_pnl[i].pnl);
    }

    trades = {
        {101, "TFS", false, 11.00, 15},
        {102, "TFS", false, 12.50, 15},
        {103, "TFS", true, 13.00, 35},
        {104, "TFS", false, 12.75, 5},
    };
    expected_pnl =
        std::vector<PnlRecord>{{103, "TFS", +13.00 * 35 - 11.00 * 15 - 12.50 * 15}, {104, "TFS", -5 * (12.75 - 13.00)}};
    pnl_records = calculate_pnl(trades, true);
    for (size_t i = 0; i < pnl_records.size(); ++i) {
        ASSERT_EQ(pnl_records[i].ts, expected_pnl[i].ts);
        ASSERT_EQ(pnl_records[i].symbol, expected_pnl[i].symbol);
        ASSERT_DOUBLE_EQ(pnl_records[i].pnl, expected_pnl[i].pnl);
    }
}