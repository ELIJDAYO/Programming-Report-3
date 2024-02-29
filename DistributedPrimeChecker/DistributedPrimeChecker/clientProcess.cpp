#include <iostream>
#include <boost/asio.hpp>

using namespace boost::asio;
using ip::tcp;
using namespace std;

int main() {
    try {
        io_context ioContext;
        tcp::socket socket(ioContext);

        // Connect to master server
        socket.connect(tcp::endpoint(ip::address::from_string("192.168.68.107"), 27015));

        // Send range to master server
        string range_threadCount = "1 1000000 128"; // Define the range here
        range_threadCount.push_back('\0'); // Add a null character at the end of the string
        boost::system::error_code error;
        boost::asio::write(socket, buffer(range_threadCount));

        // Receive response from masterServer
        //char response[1024];
        //size_t length = socket.read_some(buffer(response));
        //string numPrimes(response, length);
        //cout << "Number of primes received from masterServer: " << numPrimes << endl;

        // Close the socket
        socket.close();
    }
    catch (const exception& e) {
        cerr << "Error: " << e.what() << endl;
        return 1;
    }

    return 0;
}
