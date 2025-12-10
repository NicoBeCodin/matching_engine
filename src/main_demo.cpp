#include "../include/me/engine.hpp"
#include "../include/me/listener.hpp"
#include <iostream>

int main() {
    StdoutMatchEventListener listener;
    MatchingEngine engine(listener);

    engine.submit_limit_order({1, Side::Buy, 100, 10});
    engine.submit_limit_order({2, Side::Sell, 101, 5});
    engine.submit_limit_order({3, Side::Sell, 100, 7}); // crosses with bid 100

    engine.cancel_order(1);

    if (auto bb = engine.best_bid()) {
        std::cout << "Best bid: " << *bb << "\n";
    } else {
        std::cout << "No bid\n";
    }

    if (auto ba = engine.best_ask()) {
        std::cout << "Best ask: " << *ba << "\n";
    } else {
        std::cout << "No ask\n";
    }

    return 0;
}
