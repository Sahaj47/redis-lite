#include "server.h"
#include "../protocol/parser.h"
#include "../concurrency/thread_pool.h"
#include <iostream>
#include <string>
#include <thread>
#include <cstring>

// --- THE SHAPE-SHIFTER BLOCK ---
#ifdef _WIN32
    #include <winsock2.h>
#else
    #include <sys/socket.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    
    // Polyfills: We tell Linux to treat Windows-specific keywords as standard C++ types
    // so we don't have to rewrite the rest of our server logic!
    #define SOCKET int
    #define INVALID_SOCKET -1
    #define SOCKET_ERROR -1
    #define closesocket close
#endif
// -------------------------------

Server::Server(int port, KeyValueStore& store) : port(port), store(store) {}

void Server::start() {
    #ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "Winsock initialization failed.\n";
        return;
    }
    #endif

    SOCKET server_socket = socket(AF_INET, SOCK_STREAM, 0);
    
    sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr));
    listen(server_socket, SOMAXCONN);

    unsigned int cores = std::thread::hardware_concurrency();
    if (cores == 0) cores = 4; // Fallback just in case

    std::cout << "[INFO] Detected " << cores << " logical processors." << std::endl;
    std::cout << "[INFO] Booting Thread Pool with " << cores << " workers..." << std::endl;
    
    ThreadPool pool(cores);

    std::cout << "[INFO] Redis-Lite server initialized. Listening on port " << port << "..." << std::endl;

    while (true) {
        SOCKET client_socket = accept(server_socket, nullptr, nullptr);
        if (client_socket == INVALID_SOCKET) continue;

        // --- THE MASSIVE UPGRADE ---
        // Instead of std::thread(...).detach(), we package the client interaction 
        // into a lambda function [this, client_socket]() { ... } and toss it into the queue!
        
        pool.enqueue([this, client_socket]() {
            char buffer[1024];
            std::string client_buffer;

            while (true) {
                memset(buffer, 0, sizeof(buffer));
                int bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
                
                if (bytes_received <= 0) {
                    closesocket(client_socket);
                    break;
                }

                client_buffer += buffer;
                size_t pos;
                while ((pos = client_buffer.find('\n')) != std::string::npos) {
                    std::string raw_command = client_buffer.substr(0, pos);
                    client_buffer.erase(0, pos + 1);

                    if (!raw_command.empty() && raw_command.back() == '\r') {
                        raw_command.pop_back();
                    }

                    Command cmd = Parser::parse(raw_command);
                    std::string response;

                    if (cmd.name == "SET" && cmd.args.size() >= 2) {
                        int ttl = 0;
                        if (cmd.args.size() == 4 && cmd.args[2] == "EXPIRE") {
                            ttl = std::stoi(cmd.args[3]);
                        }
                        store.set(cmd.args[0], cmd.args[1], ttl);
                        response = "+OK\n";
                    } 
                    else if (cmd.name == "GET" && cmd.args.size() == 1) {
                        auto val = store.get(cmd.args[0]);
                        response = val ? *val + "\n" : "(nil)\n";
                    } 
                    else if (cmd.name == "DEL" && cmd.args.size() == 1) {
                        bool deleted = store.del(cmd.args[0]);
                        response = deleted ? "(integer) 1\n" : "(integer) 0\n";
                    } 
                    else if (cmd.name == "COMPACT" && cmd.args.size() == 0) {
                        store.compact_wal();
                        response = "+OK WAL Compacted\n";
                    }
                    else {
                        response = "-ERR unknown command\n";
                    }

                    send(client_socket, response.c_str(), response.length(), 0);
                }
            }
        }); 
        // -----------------------------
    }
}