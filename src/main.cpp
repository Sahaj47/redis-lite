#include "storage/kv_store.h"
#include "network/server.h"

int main() {
    // 1. Boot up the database engine
    KeyValueStore store;

    // 2. Boot up the network server on port 8080 and pass it the database
    Server server(8080, store);
    server.start();

    return 0;
}