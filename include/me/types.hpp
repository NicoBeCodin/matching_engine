#pragma once

#include <cstdint>
#include <string>

// Domain Types

enum class Side : uint8_t { Buy, Sell };

using OrderId = uint64_t;
using Price   = int64_t;  // integer ticks
using Qty     = int64_t;  // shares/units

struct OrderRequest {
    OrderId id;
    Side    side;
    Price   price;
    Qty     qty;
};

struct Trade {
    OrderId resting_id;
    OrderId taking_id;
    Price   price;
    Qty     qty;
};

struct OrderAck {
    OrderId     id;
    bool        accepted;
    std::string reason;
};
