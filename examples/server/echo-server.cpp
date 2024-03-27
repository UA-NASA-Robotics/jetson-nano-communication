#include <websocketpp/config/asio_no_tls.hpp>

#include <websocketpp/server.hpp>

#include <iostream>
#include <string>
#include <bitset>

typedef websocketpp::server<websocketpp::config::asio> server;

using websocketpp::lib::bind;
using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;

// pull out the type of messages sent by our config
typedef server::message_ptr message_ptr;

// Define a callback to handle incoming messages
void on_message(server *s, websocketpp::connection_hdl hdl, message_ptr msg)
{
    // std::cout << "on_message called with hdl: " << hdl.lock().get()
    //           << " and message: " << msg->get_payload()
    //           << std::endl;

    std::cout << msg->get_raw_payload() << std::endl;

    std::string str = msg->get_raw_payload(); // Get packet as a string
    const char *bytes = str.c_str();          // Convert that string into a dynamic char array
    // Print out the binary of the packet received
    for (std::size_t i = 0; i < str.size(); i++)
    {
        std::cout << std::bitset<8>(bytes[i]) << " ";
    }
    std::cout << std::endl;

    // Only do logic if packet size is 2 bytes
    if (str.size() == 2)
    {
        bool triggers[4];
        int leftWheel, rightWheel;
        unsigned char temp = 0b01000000;
        for (int i = 0; i < 4; i++)
        {
            triggers[i] = bytes[0] & temp;
            temp >>= 1;
        }

        unsigned int leftMagnitude = (bytes[1] & 0b01110000) >> 4;
        unsigned int rightMagnitude = bytes[1] & 0b00000111;
        bool isLeftNegative = bytes[1] & 0b10000000;
        bool isRightNegative = bytes[1] & 0b00001000;

        leftWheel = isLeftNegative ? -leftMagnitude : leftMagnitude;
        rightWheel = isRightNegative ? -rightMagnitude : rightMagnitude;

        leftWheel *= 14.28;
        rightWheel *= 14.28;

        std::cout << "Left: " << leftWheel << ",\tRight: " << rightWheel << ",\tTriggers: ";
        for (int i = 0; i < 4; i++)
        {
            std::cout << (triggers[i] ? "1" : "0") << ", ";
        }
        std::cout << std::endl;
    }
    else
    {
    }

    // check for a special command to instruct the server to stop listening so
    // it can be cleanly exited.
    if (msg->get_payload() == "stop-listening")
    {
        s->stop_listening();
        return;
    }

    try
    {
        s->send(hdl, msg->get_payload(), msg->get_opcode());
    }
    catch (websocketpp::exception const &e)
    {
        std::cout << "Echo failed because: "
                  << "(" << e.what() << ")" << std::endl;
    }
}

int main()
{
    // Create a server endpoint
    server echo_server;

    try
    {
        // Set logging settings
        echo_server.set_access_channels(websocketpp::log::alevel::all);
        echo_server.clear_access_channels(websocketpp::log::alevel::frame_payload);

        // Initialize Asio
        echo_server.init_asio();

        // Register our message handler
        echo_server.set_message_handler(bind(&on_message, &echo_server, ::_1, ::_2));

        // Listen on port 9002
        echo_server.listen(9002);

        // Start the server accept loop
        echo_server.start_accept();

        // Start the ASIO io_service run loop
        echo_server.run();
    }
    catch (websocketpp::exception const &e)
    {
        std::cout << e.what() << std::endl;
    }
    catch (...)
    {
        std::cout << "other exception" << std::endl;
    }
}