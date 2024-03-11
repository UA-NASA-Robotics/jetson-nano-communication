#include <websocketpp/client.hpp>
#include <websocketpp/config/asio_no_tls_client.hpp>

#include <iostream>
#include <string>

typedef websocketpp::client<websocketpp::config::asio_client> client;

class ClientHandler {
public: 
    // Constructor
    ClientHandler() {}

    // Connects to certain ip address and returns connection index
    int connect(std::string const &uri) {}

    void sendString(std::string message) {}
}

// Commands:
// help
// list-connections
//      ex: 0: ws://10.10.100.10:9002
// message-history
// connect <uri/ip>
// send-string
// fetch
// disconnect <index>
// quit

int main()
{
    while (true) {
        std::string command;
        std::cout << "Enter command: ";
        std::cin >> command;

        if (command.substr(0, 7) == "connect") {
            // connect ws://10.18.1789:9002
            // TODO: handle connecting to a uri
        } else if (command.substr(0, 4) == "quit") {
            std::cout << "Terminating Program";
            return 0;
        } else {
            std::cout << "Unknown command";
        }
    }

    return 0;
}