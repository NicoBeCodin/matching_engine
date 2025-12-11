#pragma once

#include "types.hpp"
#include "listener.hpp"

#include <deque>
#include <map>
#include <optional>
#include <unordered_map>

// Price-time priority:
//  - Each price level stores a FIFO queue of resting orders.
//  - We keep maps for bid/ask sides keyed by price.
//  - We maintain an id -> iterator map for O(1) cancel.

//Each price level has a Resting Order
struct RestingOrder {
    OrderId  id;
    Side     side;
    Price    price;
    Qty      remaining_qty;
    uint64_t arrival_seq;  // for debugging / tie-break, if needed
};

//The book is made up of price levels
struct PriceLevel {
    Price                 price;
    std::deque<RestingOrder> orders; // FIFO at this price
    Qty                   total_qty{0};
};

//We will eventualize virtualize this class to allow for different order bokk implementations
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
    struct Locator {
        Side side;
        Price price;
    };

    std::unordered_map<OrderId, Locator> locator_;

    IMatchEventListener& listener_;
    //Internal logic of the match
    void match_buy(const OrderRequest& req, Qty& remaining);

    void match_sell(const OrderRequest& req, Qty& remaining);

    void add_resting_order(const OrderRequest& req, Qty remaining);
    
};
