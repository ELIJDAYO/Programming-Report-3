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

std::vector<int> findPrimesInRange(int start, int end) {
    std::vector<int> primes;
    for (int i = start; i <= end; ++i) {
        if (isPrime(i)) {
            primes.push_back(i);
        };
    }
    return primes;
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

                std::vector<int> primes = findPrimesInRange(start, end);

                std::ostringstream oss;
                for (int prime : primes) {
                    oss << prime << " ";
                }
                std::string primesStr = oss.str();
                primesStr.push_back('\0');
                socket.write_some(buffer(primesStr));
                int length = primes.size();
                cout << "The size of primes: " << length << endl;
                cout << "Sent list of primes to master server." << std::endl;
            }
            socket.close();
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }

    return 0;
}
