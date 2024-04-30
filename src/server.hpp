#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
#include <iostream>
#include <bitset>
#include <string>
#include <functional>
#include <vector>

#include "types.hpp"

#define PORT 9002

using websocketpp::lib::bind;
using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;

// Enumeration to represent the different macros at their macroCode
enum Macro
{
    ESTOP,
    CANCEL_MACRO,
    FULL_EXTEND,
    FULL_RETRACT,
    CARRY_POS,
    DUMP_CYCLE,
    DUMP_WITH_MOVEMENT,
    DIG_CYCLE,
    TURN_RIGHT_45,
    TURN_LEFT_45,
    TURN
};

// Struct containing general packet date for all types
struct PacketData
{
    std::string rawMessage;
    std::vector<std::bitset<8>> rawBinary;
};

// Struct with all organized data from motion packets
struct MotionPacketData : PacketData
{
    int leftDrivePercent;
    int rightDrivePercent;
    ActuatorMotion actuator1;
    ActuatorMotion actuator2;
};

// Struct with all organized data from macro packets
struct MacroPacketData : PacketData
{
    Macro macro;
    bool pressed;
    unsigned int data;
};

class ServerHandler
{
protected:
    // Custom function type to be called when motion update packet is received
    typedef std::function<void(MotionPacketData, ServerHandler *)> MotionUpdateCallback;
    // Custom function type to be called when macro packet is received
    typedef std::function<void(MacroPacketData, ServerHandler *)> MacroCallback;
    // Custom function type to be called when the websocket disconnects or interupts
    typedef std::function<void(ServerHandler *)> DisconnectCallback;

    MotionUpdateCallback onMotionUpdate;     // Function to call when a motion packet is received
    MacroCallback onMacro;                   // Function to call when a macro packet is received
    DisconnectCallback onDisconnectCallback; // Function to call when the connection is disconnected or interupted

public:
    // The run loop to continuously run until the program stops (call in the main function of the program)
    virtual void run() = 0;

    // Update the function to call whenever a motion packet is received
    void setMotionUpdateCallback(MotionUpdateCallback onMotionUpdate)
    {
        this->onMotionUpdate = onMotionUpdate;
    }

    // Update the function to call whenever a macro packet is received
    void setMacroCallback(MacroCallback onMacro)
    {
        this->onMacro = onMacro;
    }

    // Update the function to call when the connection is disconnected or interupted
    void setDisconnectCallback(DisconnectCallback onDisconnectCallback)
    {
        this->onDisconnectCallback = onDisconnectCallback;
    }
};

class TCPServerHandler : public ServerHandler
{
private:
    // Websocket++ server class shorthand
    typedef websocketpp::server<websocketpp::config::asio> Server;
    // Websocket++ connection handle shorthand
    typedef websocketpp::connection_hdl ConnectionHandle;
    // Websocket++ message pointer shorthand
    typedef Server::message_ptr MessagePtr;

    // Websocket++ server object definition
    Server server;

    // Returns what ActuatorMotion enum item is the case based on the a and b booleans
    // a | b | motion
    // --|---|-------
    // 0 | 0 | NONE
    // 0 | 1 | RETRACTING
    // 1 | 0 | EXTENDING
    // 1 | 1 | NONE
    static ActuatorMotion getActuatorMotion(bool a, bool b)
    {
        if (a == b)
            return ActuatorMotion::NONE;
        else if (!a && b)
            return ActuatorMotion::RETRACTING;
        else
            return ActuatorMotion::EXTENDING;
    }

    // Internal message handler to call in the Websocket++ server object
    static void onMessage(TCPServerHandler *serverHandler, ConnectionHandle hdl, MessagePtr msg)
    {
        std::string str = msg->get_raw_payload(); // Get packet as a string
        const char *bytes = str.c_str();          // Convert that string into a dynamic char array
        std::vector<std::bitset<8>> binary;
        // Add bytes to the binary vector one by one
        for (std::size_t i = 0; i < str.size(); i++)
        {
            binary.push_back(std::bitset<8>(bytes[i]));
        }

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

            MotionPacketData data;
            data.rawMessage = str;
            data.rawBinary = binary;
            data.leftDrivePercent = leftWheelPercent;
            data.rightDrivePercent = rightWheelPercent;
            data.actuator1 = getActuatorMotion(triggers[2], triggers[0]);
            data.actuator2 = getActuatorMotion(triggers[3], triggers[1]);

            if (serverHandler->onMotionUpdate != nullptr)
            {
                serverHandler->onMotionUpdate(data, serverHandler);
            }
        }
        else if ((str.size() == 1 || str.size() == 2) && bytes[0] & 0b10000000)
        {
            // Decode packet as a macro packet (preprogrammed action to happen once) if packet length is 1 byte and first bit is 1
            unsigned int macroCode = (bytes[0] & 0b01111100) >> 2; // 0 < macroCode < 23; code representing macro to execute
            bool pressed = bytes[0] & 0b00000010;                  // Boolean representing if the button was pressed down

            MacroPacketData data;
            data.rawMessage = str;
            data.rawBinary = binary;
            data.macro = Macro(macroCode);
            data.pressed = pressed;
            data.data = str.size() == 2 ? bytes[1] : 0;

            if (serverHandler->onMacro != nullptr)
            {
                serverHandler->onMacro(data, serverHandler);
            }
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

    // Internal method to call on disconnect with the Websocket++ server object
    static void onDisconnect(TCPServerHandler *serverHandler)
    {
        if (serverHandler->onDisconnectCallback != nullptr)
        {
            serverHandler->onDisconnectCallback(serverHandler);
        }
    }

public:
    // Constructor to automatically set up the server
    TCPServerHandler()
    {
        try
        {
            // Add ability to restart program multiple times and have most recent instance use the address
            server.set_reuse_addr(true);

            // Set logging settings
            server.set_access_channels(websocketpp::log::alevel::all);
            server.clear_access_channels(websocketpp::log::alevel::frame_payload);

            // Initialize Asio
            server.init_asio();

            // Register our message handler
            server.set_message_handler(bind(&onMessage, this, ::_1, ::_2));
            server.set_interrupt_handler(bind(&onDisconnect, this));
            server.set_fail_handler(bind(&onDisconnect, this));
            server.set_close_handler(bind(&onDisconnect, this));
        }
        catch (websocketpp::exception const &e)
        {
            std::cout << e.what() << std::endl;
            std::terminate();
        }
        catch (...)
        {
            std::cout << "other exception" << std::endl;
            std::terminate();
        }
    }

    // Runs loop to listen for incoming connections, incoming messages, and disconnections/interuptions
    void run()
    {
        try
        {
            // Listen on port <PORT>
            server.listen(PORT);

            // Start the server accept loop
            server.start_accept();

            // Start the ASIO io_service run loop
            server.run();
        }
        catch (websocketpp::exception const &e)
        {
            std::cout << e.what() << std::endl;
            std::terminate();
        }
        catch (...)
        {
            std::cout << "other exception" << std::endl;
            std::terminate();
        }
    }
};