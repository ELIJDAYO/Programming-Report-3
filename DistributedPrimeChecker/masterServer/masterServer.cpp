#include <iostream>
#include <vector>
#include <boost/asio.hpp>
#include <thread>

using namespace std;
using namespace boost::asio;
namespace ip = boost::asio::ip;

void handleClient(ip::tcp::socket&& socket, const std::vector<ip::tcp::endpoint>& slaveEndpoints) {
    try {
        // Receive range from client
        char data[1024];
        size_t bytes_received = socket.read_some(buffer(data));
        string range(data, bytes_received);
        range.erase(std::remove(range.begin(), range.end(), '\n'), range.end()); // Remove newline character
        range.erase(std::remove(range.begin(), range.end(), '\r'), range.end()); // Remove carriage return character
        range.erase(std::remove(range.begin(), range.end(), '\0'), range.end()); // Remove null terminator
        cout << "Received range from client: " << range << endl;

        // Parse range
        istringstream iss(range);
        int start, end;
        iss >> start >> end;

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
        cout << "[Original Range]: " << start << " - " << end << endl;


        // Connect to slave servers and send ranges
        for (size_t i = 0; i < slaveEndpoints.size(); ++i) {
            ip::tcp::socket slaveSocket(socket.get_executor());
            ip::tcp::endpoint slaveEndpoint = slaveEndpoints[i];
            cout << "Init slavePoints" << endl;
            // Check if the slave server is reachable
            boost::system::error_code ec;
            slaveSocket.connect(slaveEndpoint, ec);
            cout << "Did it connect to slaveSocket?" << endl;

            if (!ec) {
                // Slave server is active, proceed with sending range
                string rangeToSend = to_string(slaveRanges[i].first) + " " + to_string(slaveRanges[i].second);
                slaveSocket.write_some(buffer(rangeToSend));

                // Keep the connection open until the slave server acknowledges receipt
                char response[1024];
                size_t bytes_received = slaveSocket.read_some(buffer(response));
                if (bytes_received > 0) {
                    cout << "Slave server at IP " << slaveEndpoint.address().to_string() << " acknowledged receipt of range." << endl;
                }

                // Close the connection after receiving acknowledgment
                slaveSocket.close();
            }
            else {
                // Slave server is not reachable
                cerr << "Error: Unable to connect to slave server at IP " << slaveEndpoint.address().to_string() << endl;
            }
        }

    }
    catch (const std::exception& e) {
        cerr << "Error: " << e.what() << endl;
    }
}

int main() {
    try {
        io_context ioContext;

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
            std::thread(handleClient, move(socket), std::ref(slaveEndpoints)).detach();
        }
    } catch (const std::exception &e) {
        cerr << "Error: " << e.what() << endl;
    }

    return 0;
}
