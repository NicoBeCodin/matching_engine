#pragma once

#include "order_book.hpp"
#include "listener.hpp"
#include "types.hpp"
#include <optional>

class MatchingEngine {
public:
    explicit MatchingEngine(IMatchEventListener& listener)
        : book_(listener) {}

    OrderAck submit_limit_order(const OrderRequest& req);
    void cancel_order(OrderId id);
    std::optional<Price> best_bid() const;
    std::optional<Price> best_ask() const;

private:
    OrderBook book_;
};

