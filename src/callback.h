

struct SmartL3Book;
struct OrderInfo {
    int order_id;
    bool is_buy;
    int size;
    double price;
};

struct SmartObCallback {
    virtual ~SmartObCallback() = default;

    virtual void onOrderAdd(const SmartL3Book &smartOrderBook,
                            const OrderInfo &orderInfo) {};
    virtual void onOrderCancel(const SmartL3Book &smartOrderBook,
                               const OrderInfo &orderInfo) {};
    virtual void onOrderModify(const SmartL3Book &smartOrderBook,
                               const OrderInfo &orderInfo) {};
    virtual void onOrderExecution(const SmartL3Book &smartOrderBook,
                                  const OrderInfo &orderInfo) {};
};