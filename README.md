
# Matching Engine (C++ Project)

## Overview

This project is a limit-order matching engine written in modern C++.  
The goal is to practice:

- Low-level and performance-oriented C++ (data structures, memory layout, build setup)
- Concepts from electronic trading and market microstructure (order books, price–time priority)

It is not intended for production use or live trading, it's just for my learning journey.

## Features

- Single-instrument limit order book
- Price–time priority matching
- Limit orders (good-till-cancel)
- Order cancellation by ID
- Query best bid / best ask
- Pluggable event listener interface (e.g. logging trades, stats)


## Build Instructions

Requires:

* CMake (3.16+)
* A C++20 compiler (e.g. g++ 11+, clang++ 12+, or MSVC with C++20 support)

### Configure and build (Release by default)

From the project root:

```bash
mkdir -p build
cd build

# Configure (defaults to Release build type)
cmake ..

# Or explicitly:
# cmake -DCMAKE_BUILD_TYPE=Release ..

# Build all targets
cmake --build . -j
```

This produces at least:

* `main_demo`
* `test_engine`

in the `build/` directory (or appropriate subfolder on your platform).

### Running the demo

From `build/`:

```bash
./main_demo
```

This runs a small scenario that:

* Submits some buy and sell orders
* Triggers matching
* Cancels an order
* Prints trades and final best bid/ask to stdout

### Running tests / stress harness

From `build/`:

To run the default test:

```bash
./test_engine 
```
This binary can be used to:

* Feed many random orders into the engine
* Exercise matching and cancel logic under load
* Inspect performance characteristics


To run a test with custom values:
```bash
./test_engine [num_ops] [cancel_prob_pct] [max_price] [max_qty] [seed]
```

## Design Notes

* Uses `std::map` for bid and ask price levels, and a FIFO container per price level.
* Each order is tracked by an ID to support O(1) cancellation via an auxiliary index.
* Matching is strictly price–time priority within a single instrument.
* The core matching logic is separated from I/O via the `IMatchEventListener` interface.

## Future Work

Potential directions to extend this project:

* Benchmarking different data structures (maps vs flat arrays, deques vs vectors)
* Custom memory allocators / object pools for resting orders
* Additional order types (market orders, IOC/FOK)
* Simple risk checks (max size/notional per account)



