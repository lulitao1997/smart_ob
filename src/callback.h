

struct SmartObCallback {
    virtual ~SmartObCallback() = default;

    virtual void onOrderAdd(OrderBook &smartOrderBook,
                            const OrderInfo &orderInfo) {};
    virtual void onOrderCancel(OrderBook &smartOrderBook,
                               const OrderInfo &orderInfo) {};
    virtual void onOrderModify(OrderBook &smartOrderBook,
                               const OrderInfo &orderInfo) {};
    virtual void onOrderExecution(OrderBook &smartOrderBook,
                                  const OrderInfo &orderInfo) {};
};