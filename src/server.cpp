#include <websocketpp/config/asio_no_tls.hpp>

#include <websocketpp/server.hpp>
#include "motor.hpp"

#include <iostream>
#include <string>
#include <bitset>
#include <thread>
#include <chrono>

#define VERSION "0.0.8"

using websocketpp::lib::bind;
using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;

typedef websocketpp::server<websocketpp::config::asio> Server;
typedef websocketpp::connection_hdl ConnectionHandle;
typedef Server::message_ptr MessagePtr;

MotorController motors;

int prevMacroCode = -1;

// Macro function to call with macroCode input (please call in a different thread)
// A macro with a lower macroCode will cancel all macros with higher codes
// Reference macros.md for more info
void doMacro(unsigned int macroCode)
{
    // TODO: implement parsing of marco code and calling correct function
}

// Define a callback to handle incoming messages
void onMessage(Server *s, ConnectionHandle hdl, MessagePtr msg)
{
    system("clear");
    std::cout << "Version: " << VERSION << std::endl;

    std::string str = msg->get_raw_payload(); // Get packet as a string
    const char *bytes = str.c_str();          // Convert that string into a dynamic char array
    // Print out the binary of the packet received
    for (std::size_t i = 0; i < str.size(); i++)
    {
        std::cout << std::bitset<8>(bytes[i]) << " ";
    }
    std::cout << std::endl;

    if (str.size() == 2 && !(bytes[0] & 0b10000000))
    {
        // Decode packet as a motion packet (manual control of motors & actuators) if length is 2 bytes and first bit is 0
        bool triggers[4]; // [left bumper, right bumper, left trigger, right trigger]
        int leftWheelPercent, rightWheelPercent;
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

        leftWheelPercent = isLeftNegative ? -leftMagnitude : leftMagnitude;
        rightWheelPercent = isRightNegative ? -rightMagnitude : rightMagnitude;

        leftWheelPercent *= 14.28 * 0.25;
        rightWheelPercent *= 14.28 * 0.75;

        std::cout << "Left: " << leftWheelPercent << ",\tRight: " << rightWheelPercent << ",\tTriggers: ";
        for (int i = 0; i < 4; i++)
        {
            std::cout << (triggers[i] ? "1" : "0") << ", ";
        }

        std::cout << /* "Macro Code: " << prevMacroCode << */ std::endl;

        motors.setDrivePercent(leftWheelPercent, rightWheelPercent);
        if (triggers[2] == 1 || triggers[0] == 1 || triggers[3] == 1 || triggers[1] == 1)
        {
            gpioWrite(RELAY_PIN, 1);
        }
        else
        {
            gpioWrite(RELAY_PIN, 0);
        }
        motors.setActuators(triggers[2], triggers[0], triggers[3], triggers[1]);
    }
    else if (str.size() == 1 && bytes[0] & 0b10000000)
    {
        // Decode packet as a macro packet (preprogrammed action to happen once) if packet length is 1 byte and first bit is 1
        unsigned int macroCode = (bytes[0] & 0b01111100) >> 2; // 0 < macroCode < 23; code representing macro to execute
        bool pressed = bytes[0] & 0b00000010;                  // Boolean representing if the button was pressed down

        std::cout << "Macro Code: " << macroCode << "\tPressed: " << pressed << std::endl;

        // attemptMacro(macroCode);
    }
    else
    {
        // Reset movement if invalid packet recieved
        motors.stopMovement();
    }

    // check for a special command to instruct the server to stop listening so
    // it can be cleanly exited.
    if (msg->get_payload() == "stop-listening")
    {
        s->stop_listening();
        motors.stopMovement();
        return;
    }

    // Commented out example code for sending packets in the future
    // try
    // {
    //     s->send(hdl, msg->get_payload(), msg->get_opcode());
    // }
    // catch (websocketpp::exception const &e)
    // {
    //     std::cout << "Echo failed because: "
    //               << "(" << e.what() << ")" << std::endl;
    // }
}

// Reset all robot motion to 0
void on_disconnect()
{
    motors.stopMovement();
    std::cout << "All actions stopped for now..." << std::endl;
}

int main()
{
    Server server; // Create a server endpoint

    try
    {
        server.set_reuse_addr(true);
        // Set logging settings
        server.set_access_channels(websocketpp::log::alevel::all);
        server.clear_access_channels(websocketpp::log::alevel::frame_payload);

        // Initialize Asio
        server.init_asio();

        // Register our message handler
        server.set_message_handler(bind(&onMessage, &server, ::_1, ::_2));
        server.set_interrupt_handler(bind(&on_disconnect));
        server.set_fail_handler(bind(&on_disconnect));
        server.set_close_handler(bind(&on_disconnect));

        // Listen on port 9002
        server.listen(9002);

        // Start the server accept loop
        server.start_accept();

        // Start the ASIO io_service run loop
        server.run();
    }
    catch (websocketpp::exception const &e)
    {
        std::cout << e.what() << std::endl;
    }
    catch (...)
    {
        std::cout << "other exception" << std::endl;
    }

    gpioTerminate();
}
