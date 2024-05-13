#pragma once

#include <iostream>
#include <string>
#include <stdlib.h>

#include "serial/serial.h"

class Arduino
{
private:
    serial::Serial interface;
    std::string portSave;
    int baudSave;
    bool connected;
    uint8_t transmissionBuffer[40];

    bool TryReconnect(std::string port, int baud)
    {
        try
        {
            interface.~Serial();

            new (&interface) serial::Serial(port, baud);
        }
        catch (serial::IOException e)
        {
            printf("No connectedo.");
            connected = false;
            return false;
        }

        connected = true;
        return true;
    }

    std::string readData()
    {

        std::string data;

        int len = interface.readline(data, 35, "\n");

        return data;
    }

public:
    Arduino()
    {
        TryReconnect("/dev/ttyACM0", 115200);
        baudSave = 115200;
        portSave = "/dev/ttyACM0";
    }
    ~Arduino() {}

    Arduino(std::string port, int baud)
    {
        TryReconnect(port, baud);
        std::cout << "testing" << std::endl;
    }

    bool readFilterdData(double *outputBuffer)
    {
        if (!connected)
            TryReconnect(portSave, baudSave);
        if (!connected)
            return 0;

        while (interface.available() < 10)
        {
            1 + 1;
        }
        std::string data = readData();

        outputBuffer[0] = atof(data.substr(0, data.find('\t')).c_str());
        outputBuffer[1] = atof(data.substr(data.find('\t') + 1, data.find('\n')).c_str());

        return 1;
    }

    void flushBuffer()
    {
        interface.flushInput();
    }
};
