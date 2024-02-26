#include <iostream>
#include <boost/asio.hpp>

using namespace boost::asio;
namespace ip = boost::asio::ip;

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
                std::string partitionedRange(data, bytesRead);
                std::cout << "Received partitioned range from master server: " << partitionedRange << std::endl;

                // TODO: Parse and process the partitioned range here
            }

            // Close the socket
            socket.close();
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        std::cin.get(); // Wait for a keypress

    }

    return 0;
}
