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
TCPServerHandler serverHandler(9003);
// UDPServerHandler udpServerHandler;

std::thread tcpThread;
std::thread macroThread;

std::string macroText = "";

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
    std::cout << macroText << std::endl;

    motors->setDrivePercent(data.leftDrivePercent * 0.25, data.rightDrivePercent * 0.25);
    motors->setActuators(data.actuator1, data.actuator2);
}

void onMacro(MacroPacketData data, ServerHandler *serverHandler)
{
    // macroText = "Executing macro " + data.macro + "\t(5=dump, 6=dig)";
    if (data.macro == Macro::DUMP_CYCLE)
    {
	    motors->dumpCycle();
    }
    else if (data.macro == Macro::DIG_CYCLE)
    {
	    motors->digCycle();
    }
}

// Reset all robot motion to 0
void onDisconnect(ServerHandler *serverHandler)
{
    motors->stopMovement();
    std::cout << "All actions stopped for now..." << std::endl;
}

void runServer(ServerHandler *serverHandler)
{
    serverHandler->run();
}

int main()
{
    // udpServerHandler.setMotionUpdateCallback(onMotionUpdate);
    // udpServerHandler.setMacroCallback(onMacro);
    // udpServerHandler.setDisconnectCallback(onDisconnect);

    serverHandler.setMotionUpdateCallback(onMotionUpdate);
    serverHandler.setMacroCallback(onMacro);
    serverHandler.setDisconnectCallback(onDisconnect);

    // tcpThread = std::thread(runServer, &serverHandler);

    serverHandler.run();

    return 0;
}
