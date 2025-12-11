
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

### Benchmarks
This section will serve to show how different optimizations have impacted the performance of this program

Initial commit:
```bash
$ ./test_engine 
=== Matching Engine Test ===
num_ops          = 1000000
cancel_prob_pct  = 10%
max_price        = 1000
max_qty          = 100
seed             = 179158905

=== Results ===
Elapsed time: 0.498953 s
Operations:   1000000
Ops/sec:      2.0042e+06
Orders accepted:   900245
Orders rejected:   0
Trades:            697427
Total traded qty:  17797576
Cancels OK:        19292
Cancels FAIL:      80463
Final best bid: 589
Final best ask: 748
```
After replacing with std::deque for order book

```bash
(base) nico@nico-ThinkPad-T490:~/Desktop/code_projects/matching_engine/build$ ./test_engine 
=== Matching Engine Test ===
num_ops          = 1000000
cancel_prob_pct  = 10%
max_price        = 1000
max_qty          = 100
seed             = 1056686254

=== Results ===
Elapsed time: 0.400675 s
Operations:   1000000
Ops/sec:      2.49579e+06
Orders accepted:   899884
Orders rejected:   0
Trades:            696699
Total traded qty:  17785288
Cancels OK:        9791
Cancels FAIL:      90325
Final best bid: 358
Final best ask: 435
```

