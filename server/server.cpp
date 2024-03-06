#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

#include <iostream>

typedef websocketpp::server<websocketpp::config::asio> server;

int main()
{
    std::cout << "Server Succesfully Ran";

    return 0;
}