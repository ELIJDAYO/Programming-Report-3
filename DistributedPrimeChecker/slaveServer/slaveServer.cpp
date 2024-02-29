#include <iostream>
#include <vector>
#include <boost/asio.hpp>
#include <algorithm> // for std::remove

using namespace std;
using namespace boost::asio;
io_context ioContext;
namespace ip = boost::asio::ip;

bool isPrime(int n) {
    if (n <= 1) return false;
    if (n <= 3) return true;
    if (n % 2 == 0 || n % 3 == 0) return false;
    for (int i = 5; i * i <= n; i += 6) {
        if (n % i == 0 || n % (i + 2) == 0) return false;
    }
    return true;
}

void findPrimesInRange(int start, int end, vector<int>& primes) {
    for (int i = start; i <= end; ++i) {
        if (isPrime(i)) {
            primes.push_back(i);
        };
    }
}

int main() {
    try {
        ip::tcp::endpoint endpoint(ip::make_address("192.168.68.108"), 27016); // Change IP and port if needed
        ip::tcp::acceptor acceptor(ioContext, endpoint);

        std::cout << "Slave server is listening on IP address: " << endpoint.address().to_string() << std::endl;
        std::cout << "Port: " << endpoint.port() << std::endl;

        while (true) {
            ip::tcp::socket socket(ioContext);
            acceptor.accept(socket);

            char data[1024];
            size_t bytesRead = socket.read_some(buffer(data));
            if (bytesRead > 0) {
                std::string rangeAndThreadCount(data, bytesRead);
                rangeAndThreadCount.erase(std::remove(rangeAndThreadCount.begin(), rangeAndThreadCount.end(), '\n'), rangeAndThreadCount.end()); // Remove newline character
                rangeAndThreadCount.erase(std::remove(rangeAndThreadCount.begin(), rangeAndThreadCount.end(), '\r'), rangeAndThreadCount.end()); // Remove carriage return character
                cout << "Received partitioned range and thread count from master server: " << rangeAndThreadCount << std::endl;

                std::istringstream iss(rangeAndThreadCount);
                int start, end, numThreads;
                if (!(iss >> start >> end >> numThreads)) {
                    cerr << "Invalid input format!" << endl;
                    continue;
                }

                // Calculate batch size
                int batchSize = 150;

                // Calculate range for each thread
                int rangeSize = (end - start + 1) / numThreads;
                int remaining = (end - start + 1) % numThreads;

                // Create vector to store primes
                vector<vector<int>> primesLists(numThreads);

                // Start threads to find primes
                vector<thread> threads;
                for (int i = 0; i < numThreads; ++i) {
                    int threadStart = start + i * rangeSize;
                    int threadEnd = threadStart + rangeSize - 1 + (i == numThreads - 1 ? remaining : 0);
                    threads.emplace_back([=, &primesLists]() {
                        findPrimesInRange(threadStart, threadEnd, primesLists[i]);
                        });
                }

                // Wait for all threads to finish
                for (auto& thread : threads) {
                    thread.join();
                }

                // Combine primes from all threads
                vector<int> combinedPrimes;
                for (const auto& primes : primesLists) {
                    combinedPrimes.insert(combinedPrimes.end(), primes.begin(), primes.end());
                }

                // Send primes in batches
                for (size_t i = 0; i < combinedPrimes.size(); i += batchSize) {
                    size_t endIndex = min(i + batchSize, combinedPrimes.size());
                    std::ostringstream oss;
                    for (size_t j = i; j < endIndex; ++j) {
                        oss << combinedPrimes[j] << " ";
                    }
                    std::string primesStr = oss.str();
                    primesStr.push_back('\0');
                    socket.write_some(buffer(primesStr)); // Send the batch
                    char ack[3];
                    socket.read_some(buffer(ack)); // Wait for acknowledgment from master server
                }
            }
            socket.close();
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }

    return 0;
}
