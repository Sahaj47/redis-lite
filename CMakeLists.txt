# Minimum version of CMake required
cmake_minimum_required(VERSION 3.10)

# Name of the project
project(RedisLite VERSION 1.0)

# Enforce C++17 standard for modern features
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Find the threading library
find_package(Threads REQUIRED)

# Define the final executable name and its source files
add_executable(redis_lite 
    src/main.cpp
    src/storage/kv_store.cpp
    src/protocol/parser.cpp
    src/network/server.cpp
    src/concurrency/thread_pool.cpp
)

# Link the threading library AND the Windows socket library
if(WIN32)
    target_link_libraries(redis_lite PRIVATE Threads::Threads ws2_32)
else()
    target_link_libraries(redis_lite PRIVATE Threads::Threads)
endif()