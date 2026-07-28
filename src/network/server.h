#pragma once
#include "../storage/kv_store.h"

class Server {
private:
    int port;
    KeyValueStore& store; // A reference to our database engine

public:
    // The constructor takes a port number and our database
    Server(int port, KeyValueStore& store);
    
    // Starts the infinite listening loop
    void start();
};