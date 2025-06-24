# Instructions

To compile

```
cmake -Bbuild .
cmake --build build
```

To test
```
./build/tests/unit_tests
```

# Assumptions

- we assume the 3 streams have continious messages and no packet drop.
- the trade and l3 execution message where send by the order from book top to bottom

# Logic

We maintain a smart orderbook by merging l3_update, l2_update, and trade by the following ways.

1. the final merged orderbook is a level3 book, consists of `L3SmartPriceLevel`, the estimated orders are calculated lazily by `GetOrders` function

2. `L3SmartPriceLevel` consists of the orders derived from (seq_id from small to large, in that order) `orders`, `l2_qty`, and `unconfirmed_trades`. this means, we know that this level has changed from only have `orders`, to having the total qty of `l2_qty`, then, after this, we'll have `unconfirmed_trades` happens in this level
3. `orders` comes from l3_updates, `l2_qty` comes from l3_updates or l2_snapshot, `unconfirmed_trades` comes from the trade messages with seq_id larger than the previous l3_update or l2_update

## Updating
- l3_update
   - update the `orders`
   - if seq_id is newest, trigger callbacks

- l2_update
    - if seq_id is before any l3_update, ignore
    - update the l2_qty of each level, remove the trade in `unconfirmed_trades` with seq_id smaller then seq_id
    - when updating, compare the new l2_qty and old l2_qty
        - if increasing, send a dummy add order callback
        - if decreasing, we can calculate how much cancel and execution between last l2_update and this l2_update by the 30% ratio and `unconfirmed_trade` we removed
        - trigger the callback according to the calculation

- trade
    - if trade's seq_id is before any l3_udpate / l2_qty, ignore
    - append trade to the `unconfirmed_trades`
    - if this trade happens, we can delete the levels that are top of this price, and possibly trigger cancel callbacks
    - trigger execution callback

## Calculating level3 orders

for each level, we remove the orders in the front, until the qty of the orders reaches `l2_qty`, or add order at the end for qty to reach `l2_qty`, then remove orders from the back until we reduced `total_unconfirmed`.

# Further Optimization

## orderbook datastruture
for orderbook, we can use `std::vector` instead of `std::map`, where bids levels where sorted by price from low to high, asks from high to low

the levels close to the top of the book are at the end of the vector.

for book insert / delete, just use std::vector's insert, delete, only the levels behind this level where moved.

since in real world, orderbook usually change near top of the book, this will perform better than `std::map`

`std::list<Order>` can be replaced with intrusive list, to remove one layer of indirection.

allocation of layers / orders struct can use memory pool