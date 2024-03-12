#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

#include <iostream>

typedef websocketpp::server<websocketpp::config::asio> server;

// Commands:
// help
// list-connections
//      ex: 0: ws://10.10.100.10:9002
// message-history
// send-string <message>
// disconnect <index>
// quit

int main()
{
    std::cout << "Server Succesfully Ran";

    return 0;
}