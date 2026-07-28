# 1. Use Alpine Linux (An incredibly tiny, lightweight OS)
FROM alpine:latest

# 2. Install the C++ compiler and CMake into this Linux OS
RUN apk add --no-cache g++ cmake make

# 3. Set the working directory inside the container
WORKDIR /app

# 4. Copy all your files from your Windows laptop into the Linux container
COPY . .

# 5. Build the project exactly like you did on Windows
RUN mkdir build && cd build && cmake .. && cmake --build .

# 6. Open the port so outside traffic can reach the database
EXPOSE 8080

# 7. Tell the container what to do when it wakes up
CMD ["./build/redis_lite"]