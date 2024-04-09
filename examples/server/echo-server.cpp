#include <websocketpp/config/asio_no_tls.hpp>

#include <websocketpp/server.hpp>
#include <unistd.h>
#include <jetgpio.h>

#include <iostream>
#include <string>
#include <bitset>
#include <thread>
#include <chrono>

#define VERSION "0.0.7"

#define GPIO_FREQUENCY 50 // Frequency in Hz to run the PWM at
#define STOP_PWM_VALUE 0.0015 * GPIO_FREQUENCY * 256 // Pulse width of the PWM value to stop the drive motors
#define NUM_PARTITIONS 0.001 * GPIO_FREQUENCY * 256 // Difference between STOP_PWM_VALUE and full fowards and full backwards PWM values 

#define RIGHT_PIN 32            // Left drive motor PWM signal - output pin from jetson
#define LEFT_PIN 33             // Right drive motor PWM signal - output pin from jetson
#define ACTUATOR_1_PIN_A 35     // Back actuator extension signal (a) - output pin from jetson
#define ACTUATOR_1_PIN_B 36     // Back actuator retraction signal (b) - output pin from jetson
#define LIM_SWITCH_1_EXT_PIN 37 // Back actuator extended position limit switch signal (1: stop) - input pin to jetson
#define LIM_SWITCH_1_CON_PIN 38 // Back actuator retracted position limit switch signal (1: stop) - input pin to jetson
#define ACTUATOR_2_PIN_A 28     // Front actuator extension signal (a) - output pin from jetson
#define ACTUATOR_2_PIN_B 29     // Front actuator retration signal (b) - output pin from jetson
#define LIM_SWITCH_2_EXT_PIN 23 // Front actuator extended position limit switch signal (1: stop) - input pin to jetson
#define LIM_SWITCH_2_CON_PIN 26 // Front actuator extended position limit switch signal (1: stop) - input pin to jetson

#define RELAY_PIN 24

typedef websocketpp::server<websocketpp::config::asio> server;

using websocketpp::lib::bind;
using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;

// pull out the type of messages sent by our config
typedef server::message_ptr message_ptr;

std::thread macroThread;
bool disableManualActuators = false; // Set to true to stop motion packets from moving actuators (so macros go correctly)
bool disableManualDrive = false;     // Set to true to stop motion packets from moving drive motors (so macros go correctly)
int prevMacroCode = -1;
int prevRightDrive, prevLeftDrive = 0;
int prevAct1a, prevAct1b, prevAct2a, prevAct2b = 0;

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

// Initialize Jetson general input output pins
// And set PWM frequency and set an initial value of wavelength
int initJetGpio()
{
    int error; // This int will store the jetpio initiallization error code

    error = gpioInitialise();

    if (error < 0)
    {
        printf("Jetgpio initialisation failed. Error code %d\n", error);
        return error;
    }
    else
    {
        printf("Jetgpio initialisation OK. Return code: %d\n", error);
    }

    gpioSetPWMfrequency(LEFT_PIN, GPIO_FREQUENCY);
    gpioSetPWMfrequency(RIGHT_PIN, GPIO_FREQUENCY);
    gpioPWM(LEFT_PIN, STOP_PWM_VALUE);
    gpioPWM(RIGHT_PIN, STOP_PWM_VALUE);
    gpioSetMode(ACTUATOR_1_PIN_A, JET_OUTPUT);
    gpioSetMode(ACTUATOR_1_PIN_B, JET_OUTPUT);
    gpioSetMode(ACTUATOR_2_PIN_A, JET_OUTPUT);
    gpioSetMode(ACTUATOR_2_PIN_B, JET_OUTPUT);
    gpioSetMode(LIM_SWITCH_1_EXT_PIN, JET_INPUT);
    gpioSetMode(LIM_SWITCH_1_CON_PIN, JET_INPUT);
    gpioSetMode(LIM_SWITCH_2_EXT_PIN, JET_INPUT);
    gpioSetMode(LIM_SWITCH_2_CON_PIN, JET_INPUT);

    gpioSetMode(RELAY_PIN, JET_OUTPUT);

    return 0;
}

// Set PWM in percent -100 <= x <= 100
void setGpioPWM(int pin, int x)
{
    if (x < -100 || x > 100)
    {
        setGpioPWM(x, 0);
        return;
    }

    int percent_to_PWM = x * NUM_PARTITIONS / 100;
    int PWM_Width = STOP_PWM_VALUE + percent_to_PWM;

    gpioPWM(pin, PWM_Width);
}

// Set the pwm percent [-100, 100] for left and right drive motors
void setWheelsPWM(int left, int right)
{
    // if (disableManualDrive)
    //     return;

    if (prevLeftDrive != left) {
        prevLeftDrive = left;
        setGpioPWM(LEFT_PIN, -left);
    }
    
    if (prevRightDrive != right) {
        prevRightDrive = right;
        setGpioPWM(RIGHT_PIN, -right);
    }
}

// Set the motion for actuator 1
// a    b   |   motion
// ------------------------
// 0    0   |   no movement !gpioRead(LIM_SWITCH_1_EXT_PIN)
// 0    1   |   contracting
// 1    0   |   extending
// 1    1   |   no movement -- try not to do this one
void setActuator1(bool a, bool b)
{
    gpioWrite(ACTUATOR_1_PIN_A, a);
    gpioWrite(ACTUATOR_1_PIN_B, b);

    // if (disableManualActuators)
    //     return;

    // if (a && !gpioRead(LIM_SWITCH_1_EXT_PIN))
    // {
    //     // Extending & not hit extend limit
    //     gpioWrite(ACTUATOR_1_PIN_A, 1);
    //     gpioWrite(ACTUATOR_1_PIN_B, 0);
    // }
    // else if (b && !gpioRead(LIM_SWITCH_1_CON_PIN))
    // {
    //     // Contracting & not hit contract limit
    //     gpioWrite(ACTUATOR_1_PIN_A, 0);
    //     gpioWrite(ACTUATOR_1_PIN_B, 1);
    // }
    // else
    // {
    //     // No movement or hit a limit
    //     gpioWrite(ACTUATOR_1_PIN_A, 0);
    //     gpioWrite(ACTUATOR_1_PIN_B, 0);
    // }
}

// Set the motion for actuator 2 (same truth table as 1)
void setActuator2(bool a, bool b)
{
    gpioWrite(ACTUATOR_2_PIN_A, a);
    gpioWrite(ACTUATOR_2_PIN_B, b);

    // if (disableManualActuators)
    //     return;

    // if (a && !gpioRead(LIM_SWITCH_2_EXT_PIN))
    // {
    //     // Extending & not hit extend limit
    //     gpioWrite(ACTUATOR_2_PIN_A, 1);
    //     gpioWrite(ACTUATOR_2_PIN_B, 0);
    // }
    // else if (b && !gpioRead(LIM_SWITCH_2_CON_PIN))
    // {
    //     // Contracting & not hit contract limit
    //     gpioWrite(ACTUATOR_2_PIN_A, 0);
    //     gpioWrite(ACTUATOR_2_PIN_B, 1);
    // }
    // else
    // {
    //     // No movement or hit a limit
    //     gpioWrite(ACTUATOR_2_PIN_A, 0);
    //     gpioWrite(ACTUATOR_2_PIN_B, 0);
    // }
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
    prevMacroCode = macroCode;
    if (disableActuatorMacros[macroCode])
        disableManualActuators = true;
    if (disableDriveMacros[macroCode])
        disableManualDrive = true;

    switch (macroCode)
    {
    case 5:
        dumpCycle();
        break;
    case 6:
        digCycle();
        break;
    }

    prevMacroCode = -1;
    disableManualActuators = false;
    disableManualDrive = false;
}

void attemptMacro(unsigned int macroCode) {
    if (prevMacroCode >= 0 && macroCode >= prevMacroCode)
        return;


    macroThread.~thread();

    try {
        macroThread = std::thread(doMacro, macroCode);
    } catch (...) {
        std::cout << "Macro Terminated" << std::endl;
    }
}

// Define a callback to handle incoming messages
void on_message(server *s, websocketpp::connection_hdl hdl, message_ptr msg)
{
    // std::cout << "on_message called with hdl: " << hdl.lock().get()
    //           << " and message: " << msg->get_payload()
    //           << std::endl;

    system("clear");
    std::cout << "Version: " << VERSION << std::endl;
    // std::cout << msg->get_raw_payload() << std::endl;

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

        leftWheel *= 14.28 / 4;
        rightWheel *= 14.28 / 4;

        std::cout << "Left: " << leftWheel << ",\tRight: " << rightWheel << ",\tTriggers: ";
        for (int i = 0; i < 4; i++)
        {
            std::cout << (triggers[i] ? "1" : "0") << ", ";
        }

        std::cout << /* "Macro Code: " << prevMacroCode << */ std::endl;

        setWheelsPWM(leftWheel, rightWheel);
        if(triggers[2]== 1 || triggers[0]== 1 || triggers[3]== 1 || triggers[1]== 1){
            gpioWrite(RELAY_PIN, 1);
        }else{
            gpioWrite(RELAY_PIN, 0);
        }
        setActuator1(triggers[2], triggers[0]);
        setActuator2(triggers[3], triggers[1]);
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
        setWheelsPWM(0, 0);
        setActuator1(0, 0);
        setActuator2(0, 0);
    }

    // check for a special command to instruct the server to stop listening so
    // it can be cleanly exited.
    if (msg->get_payload() == "stop-listening")
    {
        s->stop_listening();
        setWheelsPWM(0, 0);
        setActuator1(0, 0);
        setActuator2(0, 0);
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
    setWheelsPWM(0, 0);
    setActuator1(0, 0);
    setActuator2(0, 0);
    std::cout << "All actions stopped for now..." << std::endl;
}

int main()
{
    initJetGpio();

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
