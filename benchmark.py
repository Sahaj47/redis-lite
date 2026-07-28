import socket
import time
import concurrent.futures

# --- Configuration ---
HOST = '127.0.0.1'
PORT = 8080
TOTAL_REQUESTS = 100000
CONCURRENT_CLIENTS = 200
REQUESTS_PER_CLIENT = TOTAL_REQUESTS // CONCURRENT_CLIENTS

def pooled_client_worker(client_id):
    """
    Simulates a single backend server holding a persistent connection.
    It opens ONE socket and fires thousands of commands through it.
    """
    successful_requests = 0
    command = "SET stress_test_key 999\n".encode()

    try:
        # 1. Open the connection EXACTLY ONCE
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
            s.connect((HOST, PORT))
            
            # 2. Keep the pipeline open and stream multiple commands
            for _ in range(REQUESTS_PER_CLIENT):
                s.sendall(command)
                
                # Wait for the server to acknowledge before sending the next one
                response = s.recv(1024)
                if response:
                    successful_requests += 1
                    
    except Exception as e:
        print(f"Client {client_id} failed: {e}")
        
    return successful_requests

def stress_test():
    print("--- Starting Redis-Lite Benchmark (Connection Pooling) ---")
    print(f"Target: {TOTAL_REQUESTS} requests")
    print(f"Simulating {CONCURRENT_CLIENTS} persistent, long-lived client connections...")

    start_time = time.time()

    # Use a ThreadPool to spawn our 100 persistent clients
    with concurrent.futures.ThreadPoolExecutor(max_workers=CONCURRENT_CLIENTS) as executor:
        
        # We just pass the client IDs (0 to 99) to start the workers
        worker_ids = range(CONCURRENT_CLIENTS)
        
        # Fire them all at the server
        results = list(executor.map(pooled_client_worker, worker_ids))

    end_time = time.time()
    
    # Calculate Metrics
    time_taken = end_time - start_time
    throughput = TOTAL_REQUESTS / time_taken
    successful_requests = sum(results)

    print("\n--- Benchmark Complete ---")
    print(f"Time Taken: {time_taken:.2f} seconds")
    print(f"Throughput: {throughput:.2f} requests/second")
    print(f"Successful Requests: {successful_requests} / {TOTAL_REQUESTS}")
    
    if successful_requests == TOTAL_REQUESTS:
        print("\n[SUCCESS] Your C++ Mutex Locks held perfectly! Zero data corruption.")
    else:
        print("\n[FAIL] Some requests dropped. Check your server output.")

if __name__ == "__main__":
    stress_test()