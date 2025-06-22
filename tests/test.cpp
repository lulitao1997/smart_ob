#include <gtest/gtest.h>

#include "ob.h"
#include "smart_ob.h"
#include "stream_msg.h"
#include "types.h"

struct Mock : SmartObCallback {
    void Common(const SmartL3Book &smartOrderBook, const OrderInfo &orderInfo) {

        smartOrderBook.DebugCheck();
        std::cout << smartOrderBook.ToString() << std::endl;
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

TEST(Test, Basic) {
    Mock callback;
    SmartL3Book ob(&callback);
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

}