#pragma once

#include "types.hpp"
#include "listener.hpp"

#include <list>
#include <map>
#include <optional>
#include <unordered_map>

// Price-time priority:
//  - Each price level stores a FIFO queue of resting orders.
//  - We keep maps for bid/ask sides keyed by price.
//  - We maintain an id -> iterator map for O(1) cancel.

struct RestingOrder {
    OrderId  id;
    Side     side;
    Price    price;
    Qty      remaining_qty;
    uint64_t arrival_seq;  // for debugging / tie-break, if needed
};

struct PriceLevel {
    Price                 price;
    std::list<RestingOrder> orders; // FIFO at this price
    Qty                   total_qty{0};
};

class OrderBook {
public:
    explicit OrderBook(IMatchEventListener& listener)
        : listener_(listener) {}

    // Only limit orders, good-till-cancelled
    void add_limit_order(const OrderRequest& req);

    void cancel_order(OrderId id);

    std::optional<Price> best_bid() const ;

    std::optional<Price> best_ask() const ;

private:
    // Two separate maps: bids and asks.
    // Bids: highest price first -> std::greater
    using BidMap = std::map<Price, PriceLevel, std::greater<Price>>;
    using AskMap = std::map<Price, PriceLevel, std::less<Price>>;

    BidMap   bids_;
    AskMap   asks_;
    uint64_t arrival_counter_{0};

    // order id -> iterator info
    struct IdRef {
        Side side;
        // Pointer-ish to level and order
        std::list<RestingOrder>::iterator order_it;
        // raw iterators into map; we store separately by side
        BidMap::iterator bid_level_it; // valid if side == Buy
        AskMap::iterator ask_level_it; // valid if side == Sell
    };

    std::unordered_map<OrderId, IdRef> id_index_;

    IMatchEventListener& listener_;

    void match_buy(const OrderRequest& req, Qty& remaining);

    void match_sell(const OrderRequest& req, Qty& remaining);

    void add_resting_order(const OrderRequest& req, Qty remaining);
    
};
