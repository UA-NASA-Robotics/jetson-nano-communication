#include <websocketpp/client.hpp>
#include <websocketpp/config/asio_no_tls_client.hpp>

#include <iostream>

typedef websocketpp::client<websocketpp::config::asio_client> client;

int main()
{
    std::cout << "Client Succesfully Ran";

    return 0;
}