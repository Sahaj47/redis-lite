# Redis-Lite: High-Performance Concurrent Key-Value Store

A lightweight, multi-threaded in-memory database engineered in C++. This project relies entirely on the C++ Standard Library (avoiding heavy external frameworks) to demonstrate core systems programming, memory safety, and network protocol management.

## 🚀 Hard Metrics & Performance
* **In-Memory Throughput:** Achieved **~55,000 requests/second** during stress testing with 200 concurrent clients.
* **Persistent Throughput:** Achieved **~25,000 requests/second** with strict Write-Ahead Logging (WAL) enabled (flushing to the physical disk on every command).
* **Concurrency:** Safely handles massive traffic spikes utilizing a custom 8-worker Thread Pool dynamically mapped to the host's logical CPU cores.

## 🧠 Core Architecture
* **Thread Pool & Task Queue:** Prevents OS context-switching overhead and port starvation by reusing a fixed pool of threads managed by Condition Variables and Mutexes.
* **Granular Read-Write Locking:** Utilizes `std::shared_mutex` to allow infinite simultaneous reads (`GET`) while exclusively locking for modifications (`SET`/`DEL`), preventing race conditions and Iterator Invalidation.
* **$O(\log N)$ TTL Eviction:** Implements a Min-Heap (`std::priority_queue`) and a detached background daemon thread to actively track and evict expired keys without freezing the database (Active/Lazy Eviction hybrid).
* **Write-Ahead Log (WAL) & Compaction:** * Ensures 100% data durability by appending raw commands to a `.wal` file prior to RAM mutation.
  * **Log Compaction (`COMPACT`):** Features an AOF (Append-Only File) rewrite mechanism. By exclusively locking the database, it safely dumps a snapshot of active memory to a new file, pruning dead keys and historical data to prevent infinite disk bloat.

## 🛠️ How to Build & Run
This project uses CMake for out-of-source builds.

1. `mkdir build && cd build`
2. `cmake ..`
3. `cmake --build .`
4. Run the executable: `.\redis_lite.exe`

Connect via Telnet or Netcat: `telnet localhost 8080`
