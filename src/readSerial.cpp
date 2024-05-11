#include "serial.hpp"
#include "serial/serial.h"

std::string readSerial()
{

    std::string data;
    // Read the incoming serial data and store it in the 'data' variable
    // You can use the appropriate serial communication library for your Jetson Nano, such as 'Serial' or 'SerialPort'
    // Here's an example using the 'Serial' library:
    Serial serial("/dev/ttyUSB0", 115200); // Replace "/dev/ttyUSB0" with the correct serial port and 9600 with the appropriate baud rate
    if (serial.isOpen()) {
        data = serial.read();
        serial.close();
    } else {
        // Handle the case when the serial port fails to open
        // You can throw an exception or return an error message
        // For simplicity, let's return a default value
        data = "00000000";
    }
    return data;
}