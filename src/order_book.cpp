#include "../include/me/order_book.hpp"

// --------- OrderBook public methods --------- //


void OrderBook::add_limit_order(const OrderRequest& req) {
    // Basic validation
    if (req.qty <= 0) {
        listener_.on_order_rejected({req.id, false, "Non-positive quantity"});
        return;
    }

    // First try to match against opposite side
    Qty remaining = req.qty;
    if (req.side == Side::Buy) {
        match_buy(req, remaining);
    } else {
        match_sell(req, remaining);
    }

    if (remaining > 0) {
        // Add remaining qty as resting order
        add_resting_order(req, remaining);
        listener_.on_order_accepted({req.id, true, ""});
    } else {
        // Fully filled, still send ack
        listener_.on_order_accepted({req.id, true, ""});
    }
}

void OrderBook::cancel_order(OrderId id) {
    auto it = id_index_.find(id);
    if (it == id_index_.end()) {
        listener_.on_order_cancelled(id, false);
        return;
    }

    auto& ref = it->second;

    if (ref.side == Side::Buy) {
        auto level_it = ref.bid_level_it;
        PriceLevel& level = level_it->second;
        auto order_it = ref.order_it;

        level.total_qty -= order_it->remaining_qty;
        level.orders.erase(order_it);

        if (level.orders.empty()) {
            bids_.erase(level_it);
        }
    } else {
        auto level_it = ref.ask_level_it;
        PriceLevel& level = level_it->second;
        auto order_it = ref.order_it;

        level.total_qty -= order_it->remaining_qty;
        level.orders.erase(order_it);

        if (level.orders.empty()) {
            asks_.erase(level_it);
        }
    }

    id_index_.erase(it);
    listener_.on_order_cancelled(id, true);
}

std::optional<Price> OrderBook::best_bid() const {
    if (bids_.empty()) {
        return std::nullopt;
    }
    return bids_.begin()->first;
}

std::optional<Price> OrderBook::best_ask() const {
    if (asks_.empty()) {
        return std::nullopt;
    }
    return asks_.begin()->first;
}

// --------- OrderBook private helpers --------- //

void OrderBook::match_buy(const OrderRequest& req, Qty& remaining) {
    while (remaining > 0 && !asks_.empty()) {
        auto best_ask_it = asks_.begin();
        PriceLevel& level = best_ask_it->second;

        if (level.price > req.price) {
            // no more crossing prices
            break;
        }

        // Walk FIFO orders at this price
        auto it = level.orders.begin();
        while (it != level.orders.end() && remaining > 0) {
            RestingOrder& resting = *it;
            Qty traded = std::min(remaining, resting.remaining_qty);

            listener_.on_trade(Trade{
                resting.id,
                req.id,
                level.price,
                traded
            });

            resting.remaining_qty -= traded;
            remaining             -= traded;
            level.total_qty       -= traded;

            if (resting.remaining_qty == 0) {
                // remove from id index
                id_index_.erase(resting.id);
                it = level.orders.erase(it);
            } else {
                ++it;
            }
        }

        if (level.orders.empty()) {
            asks_.erase(best_ask_it);
        } else {
            // still resting orders at this level; stop if no more crossing
            if (level.price > req.price) {
                break;
            }
        }
    }
}

void OrderBook::match_sell(const OrderRequest& req, Qty& remaining) {
    while (remaining > 0 && !bids_.empty()) {
        auto best_bid_it = bids_.begin();
        PriceLevel& level = best_bid_it->second;

        if (level.price < req.price) {
            // no more crossing
            break;
        }

        auto it = level.orders.begin();
        while (it != level.orders.end() && remaining > 0) {
            RestingOrder& resting = *it;
            Qty traded = std::min(remaining, resting.remaining_qty);

            listener_.on_trade(Trade{
                resting.id,
                req.id,
                level.price,
                traded
            });

            resting.remaining_qty -= traded;
            remaining             -= traded;
            level.total_qty       -= traded;

            if (resting.remaining_qty == 0) {
                id_index_.erase(resting.id);
                it = level.orders.erase(it);
            } else {
                ++it;
            }
        }

        if (level.orders.empty()) {
            bids_.erase(best_bid_it);
        } else {
            if (level.price < req.price) {
                break;
            }
        }
    }
}

void OrderBook::add_resting_order(const OrderRequest& req, Qty remaining) {
    RestingOrder ro{
        req.id,
        req.side,
        req.price,
        remaining,
        ++arrival_counter_
    };

    if (req.side == Side::Buy) {
        auto [level_it, inserted] =
            bids_.try_emplace(req.price, PriceLevel{req.price, {}, 0});
        PriceLevel& level = level_it->second;
        auto order_it = level.orders.insert(level.orders.end(), ro);
        level.total_qty += remaining;

        IdRef ref;
        ref.side          = Side::Buy;
        ref.order_it      = order_it;
        ref.bid_level_it  = level_it;
        id_index_[req.id] = ref;
    } else {
        auto [level_it, inserted] =
            asks_.try_emplace(req.price, PriceLevel{req.price, {}, 0});
        PriceLevel& level = level_it->second;
        auto order_it = level.orders.insert(level.orders.end(), ro);
        level.total_qty += remaining;

        IdRef ref;
        ref.side          = Side::Sell;
        ref.order_it      = order_it;
        ref.ask_level_it  = level_it;
        id_index_[req.id] = ref;
    }
}
