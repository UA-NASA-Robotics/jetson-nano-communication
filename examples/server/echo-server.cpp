#include <websocketpp/config/asio_no_tls.hpp>

#include <websocketpp/server.hpp>
#include <unistd.h>
#include <jetgpio.h>

#include <iostream>
#include <string>
#include <bitset>
#include <thread>

#define GPIO_FREQUENCY 150
#define STOP_PWM_VALUE 0.0015 * GPIO_FREQUENCY * 256
#define NUM_PARTITIONS 0.0005 * GPIO_FREQUENCY * 256

#define LEFT_PIN 32
#define RIGHT_PIN 33
#define ACTUATOR_1_PIN_A 35
#define ACTUATOR_1_PIN_B 36
#define LIM_SWITCH_1_EXT_PIN 37
#define LIM_SWITCH_1_CON_PIN 38
#define ACTUATOR_2_PIN_A 28
#define ACTUATOR_2_PIN_B 29
#define LIM_SWITCH_2_EXT_PIN 24
#define LIM_SWITCH_2_CON_PIN 26

typedef websocketpp::server<websocketpp::config::asio> server;

using websocketpp::lib::bind;
using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;

// pull out the type of messages sent by our config
typedef server::message_ptr message_ptr;

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

    int percent_to_PWM = x * NUM_PARTITIONS / 100 - 1;
    int PWM_Width = STOP_PWM_VALUE + percent_to_PWM;

    gpioPWM(pin, PWM_Width);
}

// Set the pwm percent [-100, 100] for left and right drive motors
void setWheelsPWM(int left, int right)
{
    setGpioPWM(LEFT_PIN, left);
    setGpioPWM(RIGHT_PIN, right);
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
    if (a && !gpioRead(LIM_SWITCH_1_EXT_PIN))
    {
        // Extending & not hit extend limit
        gpioWrite(ACTUATOR_1_PIN_A, 1);
        gpioWrite(ACTUATOR_1_PIN_B, 0);
    }
    else if (b && !gpioRead(LIM_SWITCH_1_CON_PIN))
    {
        // Contracting & not hit contract limit
        gpioWrite(ACTUATOR_1_PIN_A, 0);
        gpioWrite(ACTUATOR_1_PIN_B, 1);
    }
    else
    {
        // No movement or hit a limit
        gpioWrite(ACTUATOR_1_PIN_A, 0);
        gpioWrite(ACTUATOR_1_PIN_B, 0);
    }
}

// Set the motion for actuator 2 (same truth table as 1)
void setActuator2(bool a, bool b)
{
    if (a && !gpioRead(LIM_SWITCH_2_EXT_PIN))
    {
        // Extending & not hit extend limit
        gpioWrite(ACTUATOR_2_PIN_A, 1);
        gpioWrite(ACTUATOR_2_PIN_B, 0);
    }
    else if (b && !gpioRead(LIM_SWITCH_2_CON_PIN))
    {
        // Contracting & not hit contract limit
        gpioWrite(ACTUATOR_2_PIN_A, 0);
        gpioWrite(ACTUATOR_2_PIN_B, 1);
    }
    else
    {
        // No movement or hit a limit
        gpioWrite(ACTUATOR_2_PIN_A, 0);
        gpioWrite(ACTUATOR_2_PIN_B, 0);
    }
}

void dumpCycle()
{
    // if (isDoingMacro)
    //     return;
    // isDoingMacro = true;

    // // TODO: implement dump cycle

    // isDoingMacro = false;
}

void digCycle()
{
    // if (isDoingMacro)
    //     return;
    // isDoingMacro = true;

    // // TODO: implement dig cycle

    // isDoingMacro = false;
}

void doMacro(unsigned int macroCode)
{
    switch (macroCode)
    {
    case 5:
        dumpCycle();
        break;
    case 6:
        digCycle();
        break;
    }
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

    // Only do logic if packet size is 2 bytes
    if (str.size() == 2 && !(bytes[0] & 0b10000000))
    {
        // Joystick and trigger packet (leftmost bit is 0)
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

        leftWheel *= 14.28;
        rightWheel *= 14.28;

        std::cout << "Left: " << leftWheel << ",\tRight: " << rightWheel << ",\tTriggers: ";
        for (int i = 0; i < 4; i++)
        {
            std::cout << (triggers[i] ? "1" : "0") << ", ";
        }
        std::cout << std::endl;

        setWheelsPWM(leftWheel, rightWheel);
        setActuator1(triggers[2], triggers[0]);
        setActuator2(triggers[3], triggers[1]);
    }
    else if (str.size() == 1 && bytes[0] & 0b10000000)
    {
        unsigned int btnCode = (bytes[0] & 0b01111100) >> 2;
        bool pressed = bytes[0] & 0b00000010;
    }
    else
    {
        setWheelsPWM(0, 0);
        setActuator1(0, 0);
        setActuator2(0, 0);
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