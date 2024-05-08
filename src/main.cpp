#include "motor.hpp"
#include "server.hpp"
#include "types.hpp"

#include <iostream>
#include <string>
#include <bitset>
#include <thread>
#include <chrono>

#define VERSION "0.0.8"

MotorInterface *motors = getMotorContoller();
UDPServerHandler serverHandler;

void onMotionUpdate(MotionPacketData data, ServerHandler *serverHandler)
{
    system("clear");
    std::cout << "Version: " << VERSION << std::endl;
    for (std::bitset<8> bitset : data.rawBinary)
    {
        std::cout << bitset << " ";
    }
    std::cout << std::endl;
    std::cout << "Left:\t" << data.leftDrivePercent << "%" << std::endl;
    std::cout << "Right:\t" << data.rightDrivePercent << "%" << std::endl;
    std::cout << "Actuator 1:\t" << data.actuator1 << std::endl;
    std::cout << "Actuator 2:\t" << data.actuator2 << std::endl;

    motors->setDrivePercent(data.leftDrivePercent * 0.25, data.rightDrivePercent * 0.25);
    motors->setActuators(data.actuator1, data.actuator2);
}

void onMacro(MacroPacketData data, ServerHandler *serverHandler)
{
    // TODO: implement macro handling
}

// Reset all robot motion to 0
void onDisconnect(ServerHandler *serverHandler)
{
    motors->stopMovement();
    std::cout << "All actions stopped for now..." << std::endl;
}

int main()
{
    serverHandler.setMotionUpdateCallback(onMotionUpdate);
    serverHandler.setMacroCallback(onMacro);
    serverHandler.setDisconnectCallback(onDisconnect);

    serverHandler.run();

    return 0;
}
