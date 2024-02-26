#include <iostream>
#include <boost/asio.hpp>
#include <thread>

using namespace std;
using namespace boost::asio;

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

void handleMaster(ip::tcp::socket&& socket) {
    try {
        // Receive range from master server
        char data[1024];
        socket.read_some(buffer(data));
        string range = data;
        cout << "Received range from master server: " << range << endl;

        // Parse range
        istringstream iss(range);
        int start, end;
        iss >> start >> end;

        // Calculate primes
        int numPrimes = countPrimesInRange(start, end);

        // Send result to master server
        string response = to_string(numPrimes);
        socket.write_some(buffer(response));
    }
    catch (const std::exception& e) {
        cerr << "Error: " << e.what() << endl;
    }
}

int main() {
    io_context ioContext;
    ip::tcp::acceptor acceptor(ioContext, ip::tcp::endpoint(ip::tcp::v4(), 27016));

    try {
        cout << "Slave server is running." << endl;

        while (true) {
            ip::tcp::socket socket(ioContext);
            acceptor.accept(socket);

            // Handle master server request in a separate thread
            thread(handleMaster, move(socket)).detach();
        }
    }
    catch (const std::exception& e) {
        cerr << "Error: " << e.what() << endl;
    }

    return 0;
}
