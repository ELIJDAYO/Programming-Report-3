#include <iostream>
#include <vector>
#include <boost/asio.hpp>
#include <thread>

using namespace std;
using namespace boost::asio;
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

int countPrimesInRange(int start, int end) {
    int count = 0;
    for (int i = start; i <= end; ++i) {
        if (isPrime(i)) ++count;
    }
    return count;
}

void handleClient(ip::tcp::socket&& socket) {
    try {
        // Receive range from client
        char data[1024];
        socket.read_some(buffer(data));
        string range = data;
        cout << "Received range from client: " << range << "" << endl;

        // Parse range
        istringstream iss(range);
        int start, end;
        iss >> start >> end;

        // Calculate primes
        int numPrimes = countPrimesInRange(start, end);

        // Send result to client
        string response = to_string(numPrimes);
        boost::asio::write(socket, buffer(response));

        // Close the socket after handling the client request
        socket.close();
    }
    catch (const std::exception& e) {
        cerr << "Error: " << e.what() << endl;
    }
}

int main() {
    boost::asio::io_context ioContext;

    // Specify the IPv4 address and port for the acceptor
    ip::tcp::endpoint endpoint(ip::address::from_string("192.168.68.107"), 27015);

    // Create and bind the acceptor
    ip::tcp::acceptor acceptor(ioContext, endpoint);

    // Print the IP address and port bound by the acceptor
    std::cout << "Master server is listening on IP address: " << endpoint.address().to_string() << std::endl;
    std::cout << "Port: " << endpoint.port() << std::endl;

    try {
        cout << "Master server is running." << endl;

        // Accept a single client connection and handle it
        ip::tcp::socket socket(ioContext);
        acceptor.accept(socket);
        handleClient(move(socket));
    }
    catch (const std::exception& e) {
        cerr << "Error: " << e.what() << endl;
    }

    return 0;
}
