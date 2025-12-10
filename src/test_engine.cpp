#include <iostream>
#include <random>
#include <chrono>
#include <cstdint>
#include <cstdlib>

// Include the engine implementation.
#include "../include/me/engine.hpp"

#define CORRECT_CHECK 100000

// Listener that collects statistics instead of printing every event.
class StatsListener : public IMatchEventListener {
public:
    uint64_t trades        = 0;
    uint64_t traded_qty    = 0;
    uint64_t accepted      = 0;
    uint64_t rejected      = 0;
    uint64_t cancels_ok    = 0;
    uint64_t cancels_fail  = 0;

    void on_trade(const Trade& t) override {
        ++trades;
        traded_qty += static_cast<uint64_t>(t.qty);
    }

    void on_order_accepted(const OrderAck& ack) override {
        if (ack.accepted) {
            ++accepted;
        } else {
            ++rejected;
        }
    }

    void on_order_rejected(const OrderAck& /*ack*/) override {
        ++rejected;
    }

    void on_order_cancelled(OrderId /*id*/, bool success) override {
        if (success) {
            ++cancels_ok;
        } else {
            ++cancels_fail;
        }
    }
};

static void print_usage(const char* prog) {
    std::cout << "Usage: " << prog
              << " [num_ops] [cancel_prob_pct] [max_price] [max_qty] [seed]\n\n"
              << "Parameters:\n"
              << "  num_ops          Number of operations (orders + cancel attempts), default 1000000\n"
              << "  cancel_prob_pct  Percentage of operations that are cancels (0-100), default 10\n"
              << "  max_price        Maximum price tick (min is 1), default 1000\n"
              << "  max_qty          Maximum quantity per order (min is 1), default 100\n"
              << "  seed             RNG seed (default: random_device)\n\n"
              << "Example:\n"
              << "  " << prog << " 2000000 5 5000 200 42\n";
}


int main(int argc, char** argv) {
    //Default params
    uint64_t num_ops         = 1'000'000;
    int      cancel_prob_pct = 10;      // 10% of operations are cancels
    Price    max_price       = 1000;
    Qty      max_qty         = 100;
    uint64_t seed            = std::random_device{}();

    if (argc > 1) {
        if (std::string(argv[1]) == "-h" || std::string(argv[1]) == "--help") {
            print_usage(argv[0]);
            return 0;
        }
        num_ops = std::strtoull(argv[1], nullptr, 10);
    }
    if (argc > 2) {
        cancel_prob_pct = std::atoi(argv[2]);
    }
    if (argc > 3) {
        max_price = static_cast<Price>(std::strtoll(argv[3], nullptr, 10));
    }
    if (argc > 4) {
        max_qty = static_cast<Qty>(std::strtoll(argv[4], nullptr, 10));
    }
    if (argc > 5) {
        seed = std::strtoull(argv[5], nullptr, 10);
    }

    if (cancel_prob_pct < 0) cancel_prob_pct = 0;
    if (cancel_prob_pct > 100) cancel_prob_pct = 100;
    if (max_price <= 0) max_price = 1;
    if (max_qty <= 0) max_qty = 1;

    std::cout << "=== Matching Engine Test ===\n";
    std::cout << "num_ops          = " << num_ops << "\n";
    std::cout << "cancel_prob_pct  = " << cancel_prob_pct << "%\n";
    std::cout << "max_price        = " << max_price << "\n";
    std::cout << "max_qty          = " << max_qty << "\n";
    std::cout << "seed             = " << seed << "\n";

    StatsListener listener;
    MatchingEngine engine(listener);

    std::mt19937_64 rng(seed);
    std::uniform_int_distribution<int> op_dist(0, 99);               // for cancel vs order
    std::uniform_int_distribution<int> side_dist(0, 1);              // 0 = buy, 1 = sell
    std::uniform_int_distribution<Price> price_dist(1, max_price);   // [1, max_price]
    std::uniform_int_distribution<Qty> qty_dist(1, max_qty);         // [1, max_qty]

    std::vector<OrderId> known_order_ids;
    known_order_ids.reserve(static_cast<size_t>(num_ops));

    OrderId next_id = 1;

    auto start = std::chrono::steady_clock::now();

    for (uint64_t i = 0; i < num_ops; ++i) {
        int op_choice = op_dist(rng);

        bool do_cancel = (op_choice < cancel_prob_pct) && !known_order_ids.empty();

        if (do_cancel) {
            // Random cancel attempt
            std::uniform_int_distribution<size_t> index_dist(0, known_order_ids.size() - 1);
            size_t idx = index_dist(rng);
            OrderId id_to_cancel = known_order_ids[idx];

            engine.cancel_order(id_to_cancel);
        } else {
            // New limit order
            Side side = (side_dist(rng) == 0) ? Side::Buy : Side::Sell;
            Price px  = price_dist(rng);
            Qty   qty = qty_dist(rng);

            OrderRequest req{next_id, side, px, qty};
            engine.submit_limit_order(req);

            known_order_ids.push_back(next_id);
            ++next_id;
        }

        // Optional correctness check every N operations (useful for testing correctness but 
        if ((i % CORRECT_CHECK) == 0 && i != 0) {
            auto bb = engine.best_bid();
            auto ba = engine.best_ask();
            if (bb && ba && *bb >= *ba) {
                std::cerr << "Invariant violated at i=" << i
                          << ": best_bid=" << *bb << " best_ask=" << *ba << "\n";
                return 1;
            }
        }
    }

    auto end = std::chrono::steady_clock::now();
    auto ns  = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
    double seconds = static_cast<double>(ns) / 1e9;

    std::cout << "\n=== Results ===\n";
    std::cout << "Elapsed time: " << seconds << " s\n";
    std::cout << "Operations:   " << num_ops << "\n";
    std::cout << "Ops/sec:      " << static_cast<double>(num_ops) / seconds << "\n";
    std::cout << "Orders accepted:   " << listener.accepted << "\n";
    std::cout << "Orders rejected:   " << listener.rejected << "\n";
    std::cout << "Trades:            " << listener.trades << "\n";
    std::cout << "Total traded qty:  " << listener.traded_qty << "\n";
    std::cout << "Cancels OK:        " << listener.cancels_ok << "\n";
    std::cout << "Cancels FAIL:      " << listener.cancels_fail << "\n";

    auto bb = engine.best_bid();
    auto ba = engine.best_ask();
    std::cout << "Final best bid: " << (bb ? std::to_string(*bb) : "NONE") << "\n";
    std::cout << "Final best ask: " << (ba ? std::to_string(*ba) : "NONE") << "\n";

    return 0;
}

