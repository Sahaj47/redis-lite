#include "kv_store.h"
#include <iostream>
#include <sstream>
#include <cstdio>

// Constructor
KeyValueStore::KeyValueStore() {
    // 1. RECOVERY: Read the log file to rebuild the hash table BEFORE starting the server
    load_from_wal(); 

    // 2. Open the file in Append mode so we don't overwrite the history
    wal_file.open("database.wal", std::ios::app);
    if (!wal_file.is_open()) {
        std::cerr << "[ERROR] Failed to open Write-Ahead Log!" << std::endl;
    }

    std::thread(&KeyValueStore::background_cleanup, this).detach();
}

// THE RECOVERY SYSTEM
void KeyValueStore::load_from_wal() {
    std::ifstream infile("database.wal");
    if (!infile.is_open()) return; // If the file doesn't exist yet (first boot), just return

    std::string line;
    int recovered_commands = 0;

    // Read the log file line by line
    while (std::getline(infile, line)) {
        std::istringstream iss(line);
        std::string cmd, key, value;
        iss >> cmd >> key;

        if (cmd == "SET") {
            iss >> value;
            int ttl = 0;
            if (iss >> ttl && ttl > 0) {
                // If there's a TTL, recreate the expiration data
                auto death_time = std::chrono::system_clock::now() + std::chrono::seconds(ttl);
                expiries[key] = death_time;
                ttl_heap.push({death_time, key});
            }
            store[key] = value; // Push directly to the map
            recovered_commands++;
        } 
        else if (cmd == "DEL") {
            store.erase(key);
            expiries.erase(key);
            recovered_commands++;
        }
    }
    std::cout << "[INFO] WAL Recovery Complete. Rebuilt " << recovered_commands << " operations." << std::endl;
}

void KeyValueStore::compact_wal() {
    // 1. Grab an exclusive lock. We must pause new writes while we swap the files.
    std::unique_lock lock(rw_lock); 

    // 2. Close the current active log file
    if (wal_file.is_open()) {
        wal_file.close();
    }

    // 3. Open a brand new temporary file
    std::ofstream temp_wal("temp.wal", std::ios::trunc);
    if (!temp_wal.is_open()) {
        std::cerr << "[ERROR] Failed to open temp WAL for compaction." << std::endl;
        wal_file.open("database.wal", std::ios::app); // Try to recover
        return;
    }

    auto now = std::chrono::system_clock::now();
    int keys_written = 0;

    // 4. Iterate through the exact current state of the RAM
    for (const auto& [key, value] : store) {
        auto exp_it = expiries.find(key);
        
        if (exp_it != expiries.end()) {
            // If the key has a TTL, only write it if it hasn't died yet
            if (exp_it->second > now) {
                auto remaining_ttl = std::chrono::duration_cast<std::chrono::seconds>(exp_it->second - now).count();
                temp_wal << "SET " << key << " " << value << " " << remaining_ttl << "\n";
                keys_written++;
            }
        } else {
            // Key has no TTL, write it permanently
            temp_wal << "SET " << key << " " << value << "\n";
            keys_written++;
        }
    }

    temp_wal.flush();
    temp_wal.close();

    // 5. File System Swap: Delete the old massive file, rename the tiny new one
    std::remove("database.wal");
    std::rename("temp.wal", "database.wal");

    // 6. Reopen the pipeline for the server to continue logging future commands
    wal_file.open("database.wal", std::ios::app);
    std::cout << "[INFO] WAL Compaction Complete. Pruned down to " << keys_written << " active keys." << std::endl;
}

void KeyValueStore::set(const std::string& key, const std::string& value, int ttl_seconds) {
    std::unique_lock lock(rw_lock); 

    // --- WRITE AHEAD LOG LOGIC ---
    // Rule: We MUST write to the physical disk BEFORE we update the RAM.
    if (wal_file.is_open()) {
        wal_file << "SET " << key << " " << value;
        if (ttl_seconds > 0) wal_file << " " << ttl_seconds;
        wal_file << "\n";
        wal_file.flush(); // FORCE the OS to write to the physical drive immediately
    }
    // -----------------------------

    store[key] = value;

    if (ttl_seconds > 0) {
        auto death_time = std::chrono::system_clock::now() + std::chrono::seconds(ttl_seconds);
        expiries[key] = death_time;
        ttl_heap.push({death_time, key});
    } else {
        expiries.erase(key);
    }
}

// ... (GET method remains completely unchanged since reading doesn't modify data) ...
std::optional<std::string> KeyValueStore::get(const std::string& key) {
    std::shared_lock lock(rw_lock); 
    auto it = store.find(key);
    if (it == store.end()) return std::nullopt;

    auto exp_it = expiries.find(key);
    if (exp_it != expiries.end() && std::chrono::system_clock::now() > exp_it->second) {
        return std::nullopt; 
    }
    return it->second;
}

bool KeyValueStore::del(const std::string& key) {
    std::unique_lock lock(rw_lock); 
    
    // --- WRITE AHEAD LOG LOGIC ---
    if (wal_file.is_open()) {
        wal_file << "DEL " << key << "\n";
        wal_file.flush();
    }
    // -----------------------------

    expiries.erase(key);
    return store.erase(key) > 0;
}

void KeyValueStore::background_cleanup() {
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        std::unique_lock lock(rw_lock);
        auto now = std::chrono::system_clock::now();

        while (!ttl_heap.empty() && ttl_heap.top().first <= now) {
            std::string key = ttl_heap.top().second;
            ttl_heap.pop(); 

            auto exp_it = expiries.find(key);
            if (exp_it != expiries.end() && exp_it->second <= now) {
                store.erase(key);
                expiries.erase(key);
                
                // --- WRITE AHEAD LOG LOGIC ---
                // We must log background evictions so the disk knows the key died!
                if (wal_file.is_open()) {
                    wal_file << "DEL " << key << "\n";
                    wal_file.flush();
                }
                // -----------------------------
            }
        }
    }
}