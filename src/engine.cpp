#include "../include/me/engine.hpp"

OrderAck MatchingEngine::submit_limit_order(const OrderRequest& req) {
    book_.add_limit_order(req);
    // In this simple design, OrderBook already calls listener with ack.
    // Here we just return a generic "accepted" (real engine would track more).
    return OrderAck{req.id, true, ""};
}

void MatchingEngine::cancel_order(OrderId id) {
    book_.cancel_order(id);
}

std::optional<Price> MatchingEngine::best_bid() const {
    return book_.best_bid();
}

std::optional<Price> MatchingEngine::best_ask() const {
    return book_.best_ask();
}
