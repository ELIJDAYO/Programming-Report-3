#include <iostream>
#include <vector>
#include <boost/asio.hpp>
#include <thread>
#include <future>
#include <chrono>
#include <algorithm> // for std::remove
#include <unordered_set> // for set operations

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

vector<int> handleClient(string rangeAndThreadCount, const vector<ip::tcp::endpoint>& slaveEndpoints) {
    vector<int> primeList;
    try {
        // Record the start time
        auto timeStart = std::chrono::steady_clock::now();

        std::cout << "Received range and thread count from client: " << rangeAndThreadCount << endl;

        // Parse range and thread count
        istringstream iss(rangeAndThreadCount);
        int start, end, numThreads;
        iss >> start >> end >> numThreads;

        // Print original range
        std::cout << "[Original Range]: " << start << "-" << end << endl;
        std::cout << "Number of threads: " << numThreads << endl;

        // Calculate range for master server
        int masterRangeSize = (end - start + 1) / 2;
        vector<int> masterPrimes;
        auto masterTask = std::async(std::launch::async, [&]() {
            return findPrimesInRange(start, start + masterRangeSize - 1);
            });

        // Print master range
        std::cout << "[Master Range]: " << start << "-" << start + masterRangeSize - 1 << endl;

        // Connect to slave servers and send ranges
        vector<future<vector<int>>> futures;
        for (size_t i = 0; i < slaveEndpoints.size(); ++i) {
            ip::tcp::endpoint slaveEndpoint = slaveEndpoints[i];
            futures.push_back(async([=]() {
                vector<int> slavePrimes; // Local variable to collect primes from slave server
                try {

                    io_context ioSlaveContext;
                    ip::tcp::socket slaveSocket(ioSlaveContext);
                    slaveSocket.connect(slaveEndpoint);
                    string rangeToSend = to_string(start + masterRangeSize) + " " + to_string(end) + " " + to_string(numThreads); // Send remaining range to slave server
                    slaveSocket.write_some(buffer(rangeToSend));

                    char response[1024];
                    while (true) {
                        size_t bytes = slaveSocket.read_some(buffer(response, 1024));
                        // Inside the loop where batches of primes are received
                        slaveSocket.write_some(buffer("ACK", 3)); // Send acknowledgment to slave server
                        if (bytes == 0) break; // No more data available
                        string responseStr(response, bytes);
                        istringstream iss(responseStr);
                        int num;
                        while (iss >> num) {
                            slavePrimes.push_back(num);
                        }
                    }
                    slaveSocket.close();
                }
                catch (const exception& e) {
                    cerr << "Error: " << e.what() << endl;
                }
                return slavePrimes; // Return primes from slave server
                }));
        }

        // Wait for master task to complete
        masterPrimes = masterTask.get();

        // Wait for all tasks to complete
        for (auto& future : futures) {
            vector<int> primes = future.get();
            primeList.insert(primeList.end(), primes.begin(), primes.end()); // Merge primes into primeList
        }
        // Record the end time
        auto timeEnd = std::chrono::steady_clock::now();

        // Calculate the duration
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(timeEnd - timeStart);
        std::cout << "Execution time: " << duration.count() << " milliseconds" << endl;

        std::cout << "Size 1: " << primeList.size() << " Size 2: " << masterPrimes.size() << endl;
        // Merge primes from master server and slave servers
        primeList.insert(primeList.end(), masterPrimes.begin(), masterPrimes.end());



        // Print total number of primes
        std::cout << "Total number of primes: " << primeList.size() << endl;

    }
    catch (const exception& e) {
        cerr << "Error: " << e.what() << endl;
    }
    return primeList;
}



// Function to handle client requests involving only the master server
vector<int> handleClient(string rangeAndThreadCount) {
    vector<int> primeList;
    try {
        // Record the start time
        auto timeStart = std::chrono::steady_clock::now();

        // Parse range and thread count
        istringstream iss(rangeAndThreadCount);
        int start, end, numThreads;
        if (!(iss >> start >> end >> numThreads)) {
            cerr << "Invalid input format!" << endl;
            return primeList;
        }

        // Print original range
        std::cout << "[Original Range]: " << start << "-" << end << endl;
        std::cout << "Number of threads: " << numThreads << endl;

        // Create a thread pool to compute primes locally
        vector<future<vector<int>>> localFutures;
        int rangeSize = end - start + 1;
        int partitionSize = rangeSize / numThreads;
        int remaining = rangeSize % numThreads;
        int threadStart = start;
        for (int i = 0; i < numThreads; ++i) {
            int threadEnd = threadStart + partitionSize - 1;
            if (remaining > 0) {
                threadEnd++;
                remaining--;
            }
            localFutures.push_back(async([=]() {
                return findPrimesInRange(threadStart, threadEnd);
                }));
            threadStart = threadEnd + 1;
        }

        // Wait for all local tasks to complete and collect results
        for (auto& future : localFutures) {
            vector<int> primes = future.get();
            primeList.insert(primeList.end(), primes.begin(), primes.end());
        }

        // Record the end time
        auto timeEnd = std::chrono::steady_clock::now();

        // Calculate the duration
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(timeEnd - timeStart);
        std::cout << "Execution time: " << duration.count() << " milliseconds" << std::endl;

        // Print total number of primes
        std::cout << "Total number of primes: " << primeList.size() << endl;
    }
    catch (const exception& e) {
        cerr << "Error: " << e.what() << endl;
    }
    return primeList;
}


double calculateSimilarity(const std::vector<int>& vec1, const std::vector<int>& vec2) {
    // Convert vectors to sets
    std::unordered_set<int> set1(vec1.begin(), vec1.end());
    std::unordered_set<int> set2(vec2.begin(), vec2.end());

    // Calculate intersection
    std::unordered_set<int> intersection;
    for (int num : set1) {
        if (set2.find(num) != set2.end()) {
            intersection.insert(num);
        }
    }

    // Calculate similarity
    double similarity = static_cast<double>(intersection.size()) / (set1.size() + set2.size() - intersection.size());
    return similarity;
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

            // Receive range and thread count from client
            char data[1024];
            size_t bytes_received = socket.read_some(buffer(data));
            string rangeAndThreadCount(data, bytes_received);
            rangeAndThreadCount.erase(std::remove(rangeAndThreadCount.begin(), rangeAndThreadCount.end(), '\n'), rangeAndThreadCount.end()); // Remove newline character
            rangeAndThreadCount.erase(std::remove(rangeAndThreadCount.begin(), rangeAndThreadCount.end(), '\r'), rangeAndThreadCount.end()); // Remove carriage return character
            
            // Handle client request involving both master and slave servers in a separate thread
            std::vector<int> resultWithSlave = handleClient(rangeAndThreadCount, slaveEndpoints);
            std::sort(resultWithSlave.begin(), resultWithSlave.end());

            std::vector<int> resultWithoutSlave = handleClient(rangeAndThreadCount);
            std::sort(resultWithoutSlave.begin(), resultWithoutSlave.end());

            if (resultWithSlave == resultWithoutSlave) {
                std::cout << "Both lists are exactly the same" << endl;
            }
            else {
                std::cout << "Does not match. Check for possible error." << endl;

                double similarity = calculateSimilarity(resultWithSlave, resultWithoutSlave);
                std::cout << "Similarity: " << similarity << std::endl;


            }
        }
    }
    catch (const exception& e) {
        cerr << "Error: " << e.what() << endl;
    }

    return 0;
}
