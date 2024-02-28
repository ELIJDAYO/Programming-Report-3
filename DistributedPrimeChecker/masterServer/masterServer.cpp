#include <iostream>
#include <vector>
#include <boost/asio.hpp>
#include <thread>
#include <future>

using namespace std;
using namespace boost::asio;
boost::asio::io_context ioContext;
namespace ip = boost::asio::ip;

vector<int> findPrimesInRange(int start, int end) {
    vector<int> primes;
    for (int num = start; num <= end; ++num) {
        bool isPrime = true;
        for (int i = 2; i * i <= num; ++i) {
            if (num % i == 0) {
                isPrime = false;
                break;
            }
        }
        if (isPrime && num > 1) {
            primes.push_back(num);
        }
    }
    return primes;
}

// Function to handle client requests
void handleClient(ip::tcp::socket&& socket, const std::vector<ip::tcp::endpoint>& slaveEndpoints) {
    try {
        // Record the start time
        auto timeStart = std::chrono::steady_clock::now();

        // Receive range and thread count from client
        char data[1024];
        size_t bytes_received = socket.read_some(buffer(data));
        string rangeAndThreadCount(data, bytes_received);
        rangeAndThreadCount.erase(std::remove(rangeAndThreadCount.begin(), rangeAndThreadCount.end(), '\n'), rangeAndThreadCount.end()); // Remove newline character
        rangeAndThreadCount.erase(std::remove(rangeAndThreadCount.begin(), rangeAndThreadCount.end(), '\r'), rangeAndThreadCount.end()); // Remove carriage return character
        cout << "Received range and thread count from client: " << rangeAndThreadCount << endl;

        // Parse range and thread count
        istringstream iss(rangeAndThreadCount);
        int start, end, numThreads;
        iss >> start >> end >> numThreads;

        // Print original range
        cout << "[Original Range]: " << start << "-" << end << endl;
        cout << "Number of threads: " << numThreads << endl;

        // Print number of ranges
        int numRanges = slaveEndpoints.size() + 1; // Including master range
        cout << "Number of ranges: " << numRanges << endl;

        // Calculate ranges for slaves
        vector<pair<int, int>> slaveRanges;
        int rangeSize = (end - start + 1) / numRanges;
        int remaining = (end - start + 1) % numRanges;
        int currentStart = start;
        for (int i = 0; i < numRanges; ++i) {
            int currentEnd = currentStart + rangeSize - 1;
            if (remaining > 0) {
                currentEnd++;
                remaining--;
            }
            slaveRanges.push_back({ currentStart, currentEnd });
            currentStart = currentEnd + 1;
        }

        // Print partitioned ranges
        for (size_t i = 0; i < slaveEndpoints.size(); ++i) {
            cout << "[Slave Range " << i + 1 << "]: " << slaveRanges[i].first << "-" << slaveRanges[i].second << endl;
        }

        // Connect to slave servers and send ranges
        vector<future<vector<int>>> futures;
        for (size_t i = 0; i < slaveEndpoints.size(); ++i) {
            ip::tcp::endpoint slaveEndpoint = slaveEndpoints[i];
            futures.push_back(async([=]() {
                try {
                    ip::tcp::socket slaveSocket(ioContext);
                    slaveSocket.connect(slaveEndpoint);
                    string rangeToSend = to_string(slaveRanges[i].first) + " " + to_string(slaveRanges[i].second) + " " + to_string(numThreads); // Include numThreads in range message
                    slaveSocket.write_some(buffer(rangeToSend));
                    char response[1024];
                    size_t bytes_received = slaveSocket.read_some(buffer(response));
                    slaveSocket.close();
                    string responseStr(response, bytes_received);
                    istringstream iss(responseStr);
                    int num;
                    vector<int> primes;
                    while (iss >> num) {
                        primes.push_back(num);
                    }
                    return primes;
                }
                catch (const exception& e) {
                    cerr << "Error: " << e.what() << endl;
                    return vector<int>();
                }
                }));
        }

        // Create a thread pool to compute primes locally
        vector<future<vector<int>>> localFutures;
        rangeSize = slaveRanges.back().second - slaveRanges.back().first + 1;
        int partitionSize = rangeSize / numThreads;
        remaining = rangeSize % numThreads;
        int threadStart = slaveRanges.back().first + 1;
        for (int i = 0; i < numThreads; ++i) {
            int threadEnd = threadStart + partitionSize - 1;
            if (remaining > 0) {
                threadEnd++;
                remaining--;
            }
            if (threadEnd > slaveRanges.back().second) {
                threadEnd = slaveRanges.back().second;
            }
            localFutures.push_back(async([=]() {
                return findPrimesInRange(threadStart, threadEnd);
                }));
            threadStart = threadEnd + 1;
        }

        // Wait for all tasks to complete and collect results
        vector<int> primeList;
        for (auto& future : futures) {
            vector<int> primes = future.get();
            primeList.insert(primeList.end(), primes.begin(), primes.end());
        }

        // Wait for all local tasks to complete and collect results
        for (auto& future : localFutures) {
            vector<int> primes = future.get();
            primeList.insert(primeList.end(), primes.begin(), primes.end());
        }
        for (int primes : primeList) {
            cout << primes << endl;
        }
        // Record the end time
        auto timeEnd = std::chrono::steady_clock::now();

        // Calculate the duration
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(timeEnd - timeStart);
        std::cout << "Execution time: " << duration.count() << " milliseconds" << std::endl;

        // Print total number of primes
        cout << "Total number of primes: " << primeList.size() << endl;

    }
    catch (const exception& e) {
        cerr << "Error: " << e.what() << endl;
    }
}

int main() {
    try {
        // Specify the IPv4 address and port for the acceptor
        ip::tcp::endpoint endpoint(ip::make_address("192.168.68.107"), 27015);

        // Create and bind the acceptor
        ip::tcp::acceptor acceptor(ioContext, endpoint);

        // Print the IP address and port bound by the acceptor
        std::cout << "Master server is listening on IP address: " << endpoint.address().to_string() << std::endl;
        std::cout << "Port: " << endpoint.port() << std::endl;

        // Specify the IP addresses and ports of the slave servers
        std::vector<ip::tcp::endpoint> slaveEndpoints = {
            ip::tcp::endpoint(ip::make_address("192.168.68.108"), 27016), // Example slave server
            // Add more slave servers here if needed
        };

        while (true) {
            // Accept connection
            ip::tcp::socket socket(ioContext);
            acceptor.accept(socket);

            // Handle client request in a separate thread
            std::thread(handleClient, move(socket), std::cref(slaveEndpoints)).detach();
        }
    }
    catch (const exception& e) {
        cerr << "Error: " << e.what() << endl;
    }

    return 0;
}
