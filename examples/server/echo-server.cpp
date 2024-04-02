#include <websocketpp/config/asio_no_tls.hpp>

#include <websocketpp/server.hpp>
#include <unistd.h>
#include <jetgpio.h>
#include "Motor.h"

#include <iostream>
#include <string>
#include <bitset>
#include <thread>
#include <chrono>

#define GPIO_FREQUENCY 150 // Cut these, (these are in th class def)
#define STOP_PWM_VALUE 0.0015 * GPIO_FREQUENCY * 256
#define NUM_PARTITIONS 0.0005 * GPIO_FREQUENCY * 256

#define LEFT_PIN 32             // Left drive motor PWM signal - output pin from jetson
#define RIGHT_PIN 33            // Right drive motor PWM signal - output pin from jetson
#define ACTUATOR_1_PIN_A 35     // Back actuator extension signal (a) - output pin from jetson
#define ACTUATOR_1_PIN_B 36     // Back actuator retraction signal (b) - output pin from jetson
#define LIM_SWITCH_1_EXT_PIN 37 // Back actuator extended position limit switch signal (1: stop) - input pin to jetson
#define LIM_SWITCH_1_CON_PIN 38 // Back actuator retracted position limit switch signal (1: stop) - input pin to jetson
#define ACTUATOR_2_PIN_A 28     // Front actuator extension signal (a) - output pin from jetson
#define ACTUATOR_2_PIN_B 29     // Front actuator retration signal (b) - output pin from jetson
#define LIM_SWITCH_2_EXT_PIN 24 // Front actuator extended position limit switch signal (1: stop) - input pin to jetson
#define LIM_SWITCH_2_CON_PIN 26 // Front actuator extended position limit switch signal (1: stop) - input pin to jetson

// Initialize Jetson general input output pins
// as well as motor Objects
Motor::initJetGpio();
Motor left_Drive_Motor(LEFT_PIN);
Motor right_Drive_Motor(RIGHT_PIN);
Motor Actuator_1(ACTUATOR_1_PIN_A, ACTUATOR_1_PIN_B, LIM_SWITCH_1_EXT_PIN, LIM_SWITCH_1_CON_PIN);
Motor Actuator_1(ACTUATOR_2_PIN_A, ACTUATOR_2_PIN_B, LIM_SWITCH_2_EXT_PIN, LIM_SWITCH_2_CON_PIN);

typedef websocketpp::server<websocketpp::config::asio> server;

using websocketpp::lib::bind;
using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;

// pull out the type of messages sent by our config
typedef server::message_ptr message_ptr;

std::thread macroThread;
bool disableManualActuators = false; // Set to true to stop motion packets from moving actuators (so macros go correctly)
bool disableManualDrive = false;     // Set to true to stop motion packets from moving drive motors (so macros go correctly)
unsigned int prevMacroCode = -1;

// Which macroCodes should disable the actuators
// Index of boolean goes relates to a macroCode
const bool disableActuatorMacros[] = {
    false, // 0: E-stop
    false, // 1: Cancel macro
    true,  // 2: Actuators to carry position
    true,  // 3: Actuators to fully retracted position
    true,  // 4: Actuators to fully extended/erect position
    true,  // 5: Full dump cycle
    true   // 6: Full dig cycle
};

// Which macroCodes should disable the drive motors
// Index of boolean goes relates to a macroCode
const bool disableDriveMacros[] = {
    false, // 0: E-stop
    false, // 1: Cancel macro
    false, // 2: Actuators to carry position
    false, // 3: Actuators to fully retracted position
    false, // 4: Actuators to fully extended/erect position
    false, // 5: Full dump cycle
    false  // 6: Full dig cycle
};


void stopAllMotors(){
    left_Drive_Motor.runDrive(0);
    right_Drive_Motor.runDrive(0);
    actuator1.setActuator(0, 0);
    actuator2.setActuator(0, 0);
}

// Disable manual actuator control and do a full bucket dump cycle
void dumpCycle()
{
    // Retract actuator 1 in
    // Extend actuator 2 out

    // Test prints
    for (int i = 0; i < 100; i++)
    {
        std::cout << "Dump Cycle: " << i << "%" << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

// Disable manual actuator control and do a full bucket dig cycle
void digCycle()
{
    // Extend both actuators

    // Test prints
    for (int i = 0; i < 100; i++)
    {
        std::cout << "Dig Cycle: " << i << "%" << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

// Macro function to call with macroCode input (please call in a different thread)
// A macro with a lower macroCode will cancel all macros with higher codes
// 0: E-stop
// 1: Cancel current macro
// 2: Set actuators to carry position (middle)
// 3: Set actuators to fully retracted position
// 4: Set actuators to fully extended/erect position
// 5: Full bucket dump cycle
// 6: Full bucket dig cycle
void doMacro(unsigned int macroCode)
{
    if (prevMacroCode >= 0 && macroCode >= prevMacroCode)
        return;

    macroThread.~thread();

    prevMacroCode = macroCode;
    if (disableActuatorMacros[macroCode])
        disableManualActuators = true;
    if (disableDriveMacros[macroCode])
        disableManualDrive = true;

    switch (macroCode)
    {
    case 5:
        macroThread = std::thread(dumpCycle);
        break;
    case 6:
        macroThread = std::thread(digCycle);
        break;
    }

    prevMacroCode = -1;
    disableManualActuators = false;
    disableManualDrive = false;
}

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

        leftWheelPercent *= 14.28;
        rightWheelPercent *= 14.28;

        std::cout << "Left: " << leftWheelPercent << ",\tRight: " << rightWheelPercent << ",\tTriggers: ";
        for (int i = 0; i < 4; i++)
        {
            std::cout << (triggers[i] ? "1" : "0") << ", ";
        }
        std::cout << std::endl;

        if (!disableManualDrive){
            left_Drive_Motor.runDrive(leftWheelPercent);
            right_Drive_Motor.runDrive(rightWheelPercent);
        }
        if (!disableManualActuators){
            actuator1.setActuator(triggers[2], triggers[0]);
            actuator2.setActuator(triggers[3], triggers[1]);
        }
    }
    else if (str.size() == 1 && bytes[0] & 0b10000000)
    {
        // Decode packet as a macro packet (preprogrammed action to happen once) if packet length is 1 byte and first bit is 1
        unsigned int macroCode = (bytes[0] & 0b01111100) >> 2; // 0 < macroCode < 23; code representing macro to execute
        bool pressed = bytes[0] & 0b00000010;                  // Boolean representing if the button was pressed down

        std::cout << "Macro Code: " << macroCode << "\tPressed: " << pressed << std::endl;

        doMacro(macroCode);
    }
    else
    {
        // Reset movement if invalid packet recieved
        stopAllMotors();
    }

    // check for a special command to instruct the server to stop listening so
    // it can be cleanly exited.
    if (msg->get_payload() == "stop-listening")
    {
        s->stop_listening();
        stopAllMotors();
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
    stopAllMotors();
    std::cout << "All actions stopped for now..." << std::endl;
}

int main()
{

    // Create a server endpoint
    server echo_server;

    try
    {
        echo_server.set_reuse_addr(true);
        // Set logging settings
        echo_server.set_access_channels(websocketpp::log::alevel::all);
        echo_server.clear_access_channels(websocketpp::log::alevel::frame_payload);

        // Initialize Asio
        echo_server.init_asio();

        // Register our message handler
        echo_server.set_message_handler(bind(&on_message, &echo_server, ::_1, ::_2));
        echo_server.set_interrupt_handler(bind(&on_disconnect));
        echo_server.set_fail_handler(bind(&on_disconnect));
        echo_server.set_close_handler(bind(&on_disconnect));

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

    gpioTerminate();
}