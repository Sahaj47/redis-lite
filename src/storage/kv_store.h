#pragma once
#include <unordered_map>
#include <string>
#include <shared_mutex>
#include <mutex>
#include <optional>
#include <queue>
#include <chrono>
#include <thread>
#include <vector>
#include <fstream>

class KeyValueStore {
private:
    std::unordered_map<std::string, std::string> store;
    std::unordered_map<std::string, std::chrono::system_clock::time_point> expiries;

    using ExpiryNode = std::pair<std::chrono::system_clock::time_point, std::string>;
    std::priority_queue<ExpiryNode, std::vector<ExpiryNode>, std::greater<ExpiryNode>> ttl_heap;

    mutable std::shared_mutex rw_lock;
    
    std::ofstream wal_file; 
    void load_from_wal();

    void background_cleanup();

public:
    KeyValueStore();

    void set(const std::string& key, const std::string& value, int ttl_seconds = 0);
    std::optional<std::string> get(const std::string& key);
    bool del(const std::string& key);
    void compact_wal();
};