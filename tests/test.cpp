#include <gtest/gtest.h>

#include "ob.h"
#include "smart_ob.h"
#include "stream_msg.h"
#include "types.h"

struct Mock : SmartObCallback {
    void Common(const SmartL3Book &smartOrderBook, const OrderInfo &orderInfo) {

        // std::cout << smartOrderBook.ToString() << std::endl;
        smartOrderBook.DebugCheck();
        ob = smartOrderBook.ToString();
        infos.push_back(orderInfo);
    }
    void onOrderAdd(const SmartL3Book &smartOrderBook,
                    const OrderInfo &orderInfo) {
        Common(smartOrderBook, orderInfo);
    };
    void onOrderCancel(const SmartL3Book &smartOrderBook,
                       const OrderInfo &orderInfo) {

        Common(smartOrderBook, orderInfo);
    };
    void onOrderModify(const SmartL3Book &smartOrderBook,
                       const OrderInfo &orderInfo) {

        Common(smartOrderBook, orderInfo);
    };
    void onOrderExecution(const SmartL3Book &smartOrderBook,
                          const OrderInfo &orderInfo) {

        Common(smartOrderBook, orderInfo);
    };

    std::vector<OrderInfo> infos;
    std::string ob;
};

void setup(Mock &m, SmartL3Book &ob) {
    ob.UpdateL3(Level3{1, level3::Add{1001, true, 10, 100.0}});
    ob.UpdateL3(Level3{2, level3::Add{1002, true, 10, 101.0}});
    ob.UpdateL3(Level3{3, level3::Add{1003, true, 10, 99.0}});
    ob.UpdateL3(Level3{4, level3::Add{1004, true, 10, 102.0}});
    ob.UpdateL3(Level3{5, level3::Add{1005, false, 10, 103.0}});
    ob.UpdateL3(Level3{6, level3::Add{1006, false, 10, 104.0}});
    ob.UpdateL3(Level3{7, level3::Add{1007, false, 10, 105.0}});
    ob.UpdateL3(Level3{8, level3::Add{1008, false, 10, 106.0}});
    ob.UpdateL3(Level3{9, level3::Cancel{1002, true}});
    ob.UpdateL3(Level3{10, level3::Modify{1003, true, 5, 99.1}});
    ob.UpdateL3(Level3{13, level3::Execute{1004, true, 3}});
}

auto ob_str = R"(BID:
102.000000:[7@1004]
100.000000:[10@1001]
99.100000:[5@1003]
ASK:
103.000000:[10@1005]
104.000000:[10@1006]
105.000000:[10@1007]
106.000000:[10@1008]
)";

TEST(Test, Basic) {
    Mock m;
    SmartL3Book ob(&m);
    setup(m, ob);
    EXPECT_EQ(m.ob, ob_str);
}

TEST(Test, Basic2) {
    Mock m;
    SmartL3Book ob(&m);
    setup(m, ob);
    ob.UpdateL3(Level3{14, level3::Add{1100, true, 3, 100.0}});
    EXPECT_EQ(m.ob, R"(BID:
102.000000:[7@1004]
100.000000:[10@1001, 3@1100]
99.100000:[5@1003]
ASK:
103.000000:[10@1005]
104.000000:[10@1006]
105.000000:[10@1007]
106.000000:[10@1008]
)");
}

TEST(Test, L2SlowerThanL3) {
    Mock m;
    SmartL3Book ob(&m);
    setup(m, ob);

    ob.UpdateL2(Snapshot{
        12,
        {{102, 7}, {100.0, 10}, {99.1, 5}},                  // Bids
        {{103.0, 10}, {104.0, 10}, {105.0, 10}, {106.0, 10}} // Asks
    });

    EXPECT_EQ(m.ob, ob_str);
    EXPECT_EQ(m.infos.size(), 11);
}

TEST(Test, L2LeadsL3) {
    Mock m;
    SmartL3Book ob(&m);
    setup(m, ob);

    // std::cout << m.ob << std::endl;

    ob.UpdateL2(Snapshot{
        20,
        {{100.0, 10}, {99.1, 5}},                            // Bids
        {{103.0, 10}, {104.0, 10}, {105.0, 10}, {106.0, 10}} // Asks
    });
    EXPECT_EQ(m.infos.size(), 13);
    EXPECT_EQ(m.ob, R"(BID:
100.000000:[10@1001]
99.100000:[5@1003]
ASK:
103.000000:[10@1005]
104.000000:[10@1006]
105.000000:[10@1007]
106.000000:[10@1008]
)");
    std::cout << m.ob << std::endl;
}


TEST(Test, L3LeadsTrade) {
    Mock m;
    SmartL3Book ob(&m);
    setup(m, ob);

    // duplicate as l3
    ob.UpdateTrade(Trade{13, true, 102.0, 3});
    EXPECT_EQ(m.ob, ob_str);
    EXPECT_EQ(m.infos.size(), 11);
}

TEST(Test, TradeLeadsL3) {
    Mock m;
    SmartL3Book ob(&m);
    setup(m, ob);


    // BID:
    // 102.000000:[7@1004]
    // 100.000000:[10@1001]
    // 99.100000:[5@1003]
    // ASK:
    // 103.000000:[10@1005]
    // 104.000000:[10@1006]
    // 105.000000:[10@1007]
    // 106.000000:[10@1008]


    // we received a trade first before l3
    ob.UpdateTrade(Trade{18, true, 102.0, 7});
    ob.UpdateTrade(Trade{19, true, 100.0, 3});


    EXPECT_EQ(ob.ToString(), R"(BID:
100.000000:[7@1001]
99.100000:[5@1003]
ASK:
103.000000:[10@1005]
104.000000:[10@1006]
105.000000:[10@1007]
106.000000:[10@1008]
)");

    ob.UpdateL3(Level3{19, level3::Execute{1001, true, 3}});

    ob.UpdateL3(Level3{14, level3::Add{1100, true, 3, 100.0}});
    ob.UpdateL3(Level3{15, level3::Add{1101, true, 3, 100.0}});
    ob.UpdateL3(Level3{16, level3::Add{1102, true, 3, 100.0}});
    ob.UpdateL3(Level3{17, level3::Add{1103, true, 3, 100.0}});
    // BID:
    // 102.000000:[7@1004]
    // 100.000000:[10@1001, 3@1100, 3@1101, 3@1102, 3@1103]
    // 99.100000:[5@1003]
    // ASK:
    // 103.000000:[10@1005]
    // 104.000000:[10@1006]
    // 105.000000:[10@1007]
    // 106.000000:[10@1008]
    ob.UpdateL3(Level3{18, level3::Add{1103, true, 3, 100.0}});
    ob.UpdateL3(Level3{19, level3::Execute{1001, true, 3}});

    EXPECT_EQ(m.ob, R"(BID:
100.000000:[7@1001]
99.100000:[5@1003]
ASK:
103.000000:[10@1005]
104.000000:[10@1006]
105.000000:[10@1007]
106.000000:[10@1008]
)");
}

TEST(Test, L2LeadsTradeLeadsL3) {
    Mock m;
    SmartL3Book ob(&m);
    setup(m, ob);


    // BID:
    // 102.000000:[7@1004]
    // 100.000000:[10@1001]
    // 99.100000:[5@1003]
    // ASK:
    // 103.000000:[10@1005]
    // 104.000000:[10@1006]
    // 105.000000:[10@1007]
    // 106.000000:[10@1008]

    ob.UpdateL2(Snapshot{
        20,
        {{100.0, 7}, {99.1, 5}},                            // Bids
        {{103.0, 10}, {104.0, 10}, {105.0, 10}, {106.0, 10}} // Asks
    });

    // we received a trade first before l3
    ob.UpdateTrade(Trade{18, true, 102.0, 7});
    ob.UpdateTrade(Trade{19, true, 100.0, 3});


    EXPECT_EQ(ob.ToString(), R"(BID:
100.000000:[7@1001]
99.100000:[5@1003]
ASK:
103.000000:[10@1005]
104.000000:[10@1006]
105.000000:[10@1007]
106.000000:[10@1008]
)");

    ob.UpdateL3(Level3{19, level3::Execute{1001, true, 3}});

    ob.UpdateL3(Level3{14, level3::Add{1100, true, 3, 100.0}});
    ob.UpdateL3(Level3{15, level3::Add{1101, true, 3, 100.0}});
    ob.UpdateL3(Level3{16, level3::Add{1102, true, 3, 100.0}});
    ob.UpdateL3(Level3{17, level3::Add{1103, true, 3, 100.0}});
    // BID:
    // 102.000000:[7@1004]
    // 100.000000:[10@1001, 3@1100, 3@1101, 3@1102, 3@1103]
    // 99.100000:[5@1003]
    // ASK:
    // 103.000000:[10@1005]
    // 104.000000:[10@1006]
    // 105.000000:[10@1007]
    // 106.000000:[10@1008]
    ob.UpdateL3(Level3{18, level3::Add{1103, true, 3, 100.0}});
    ob.UpdateL3(Level3{19, level3::Execute{1001, true, 3}});

    EXPECT_EQ(m.ob, R"(BID:
100.000000:[7@1001]
99.100000:[5@1003]
ASK:
103.000000:[10@1005]
104.000000:[10@1006]
105.000000:[10@1007]
106.000000:[10@1008]
)");
}