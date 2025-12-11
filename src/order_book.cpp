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
    auto loc_it = locator_.find(id);
    if (loc_it == locator_.end()) {
        listener_.on_order_cancelled(id, false);
        return;
    }

    const Side side = loc_it->second.side;
    const Price price = loc_it->second.price;

    bool success = false;

    if (side == Side::Buy) {
        auto level_it = bids_.find(price);
        
        if (level_it == bids_.end()){
            PriceLevel& level = level_it->second;
            auto it = level.orders.begin();
            while (it != level.orders.end()) {
                if (it->id == id){
                    level.total_qty -= it->remaining_qty;
                    it = level.orders.erase(it);
                    success = true;
                    break;
                } else {
                    ++it;
                }
            }
            if (level.orders.empty()){
                bids_.erase(level_it);
            }
            
        } 
    } else {
        auto level_it = asks_.find(price);
        if (level_it != asks_.end()){
            PriceLevel& level = level_it->second;
            auto it = level.orders.begin();
            while (it != level.orders.end()){
                if (it->id == id){
                    level.total_qty -= it -> remaining_qty;
                    it = level.orders.erase(it);
                    success =true;
                    break;
                } else {
                    ++it;
                }
            }
            if (level.orders.empty()){
                asks_.erase(level_it);
            }
         }
    }
    if (success){
        locator_.erase(loc_it);
    }

    listener_.on_order_cancelled(id, success);
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

// OrderBook private helpers

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
                // remove from locator
                locator_.erase(resting.id);
                it = level.orders.erase(it);
            } else {
                ++it;
            }
        }
        
        if (level.orders.empty()) {
            asks_.erase(best_ask_it);
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

            if (resting.remaining_qty == 0){
                locator_.erase(resting.id);
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
        
        locator_[req.id] = Locator{Side::Buy, req.price};
        
    } else {
        auto [level_it, inserted] =
            asks_.try_emplace(req.price, PriceLevel{req.price, {}, 0});
        PriceLevel& level = level_it->second;
        auto order_it = level.orders.insert(level.orders.end(), ro);
        level.total_qty += remaining;

        locator_[req.id] = Locator{Side::Sell, req.price};

    }
}
