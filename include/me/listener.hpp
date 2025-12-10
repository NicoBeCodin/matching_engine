#pragma once

#include "types.hpp"
#include <iostream>

// Event Listener Interface

struct IMatchEventListener {
    virtual ~IMatchEventListener() = default;

    virtual void on_trade(const Trade& t)                         = 0;
    virtual void on_order_accepted(const OrderAck& ack)           = 0;
    virtual void on_order_rejected(const OrderAck& ack)           = 0;
    virtual void on_order_cancelled(OrderId id, bool success)     = 0;
};

// Simple stdout listener
class StdoutMatchEventListener : public IMatchEventListener {
public:
    void on_trade(const Trade& t) override {
        std::cout << "TRADE: resting=" << t.resting_id
                  << " taking=" << t.taking_id
                  << " qty=" << t.qty
                  << " px=" << t.price << "\n";
    }

    void on_order_accepted(const OrderAck& ack) override {
        std::cout << "ORDER ACCEPTED: id=" << ack.id << "\n";
    }

    void on_order_rejected(const OrderAck& ack) override {
        std::cout << "ORDER REJECTED: id=" << ack.id
                  << " reason=" << ack.reason << "\n";
    }

    void on_order_cancelled(OrderId id, bool success) override {
        std::cout << "ORDER CANCEL " << (success ? "OK" : "FAIL")
                  << ": id=" << id << "\n";
    }
};
