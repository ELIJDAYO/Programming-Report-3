#include <iostream>
#include <vector>
#include <boost/asio.hpp>

using namespace std;
using namespace boost::asio;
namespace ip = boost::asio::ip;

// Function to check if a number is prime
bool isPrime(int n) {
    if (n <= 1) return false;
    if (n <= 3) return true;
    if (n % 2 == 0 || n % 3 == 0) return false;
    for (int i = 5; i * i <= n; i += 6) {
        if (n % i == 0 || n % (i + 2) == 0) return false;
    }
    return true;
}

// Function to compute the number of primes within a range and return the list of primes
std::vector<int> findPrimesInRange(int start, int end) {
    std::vector<int> primes;
    for (int i = start; i <= end; ++i) {
        if (isPrime(i)) { 
            //cout << i << endl;
            primes.push_back(i);
        };

    }
    return primes;
}

int main() {
    try {
        io_context ioContext;

        // Specify the IPv4 address and port for the acceptor
        ip::tcp::endpoint endpoint(ip::make_address("192.168.68.108"), 27016); // Change port if needed

        // Create and bind the acceptor
        ip::tcp::acceptor acceptor(ioContext, endpoint);

        // Print the IP address and port bound by the acceptor
        std::cout << "Slave server is listening on IP address: " << endpoint.address().to_string() << std::endl;
        std::cout << "Port: " << endpoint.port() << std::endl;

        // Main loop to continuously accept connections and process data
        while (true) {
            // Accept connection
            ip::tcp::socket socket(ioContext);
            acceptor.accept(socket);

            // Receive partitioned range from master server
            char data[1024];
            size_t bytesRead = socket.read_some(buffer(data));
            if (bytesRead > 0) {
                std::string range(data, bytesRead);
                std::cout << "Received partitioned range from master server: " << range << std::endl;

                // Parse and process the partitioned range
                std::istringstream iss(range);
                int start, end;
                iss >> start >> end;

                // Find primes in the range
                std::vector<int> primes = findPrimesInRange(start, end);

                // Send the list of primes to the master server
                std::ostringstream oss;
                for (int prime : primes) {
                    oss << prime << " ";
                }
                std::string primesStr = oss.str();
                primesStr.push_back('\0');
                socket.write_some(buffer(primesStr));
                std::cout << "Sent list of primes to master server." << std::endl;
            }

            // Close the socket
            socket.close();
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }

    return 0;
}
