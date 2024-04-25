// https://github.com/Rubberazer/JETGPIO/blob/main
//
//  1) To run first compile with the following: g++ -Wall -o motorControlTest main.cpp -ljetgpio
//     NOTE: Where motorControlTest is the output file, main.cpp is the file your compiling, and -ljetgpio is the linked file
//
//  2) Now to actually run the program run the following: sudo ./motorControlTest
//

#include <iostream>
#include <unistd.h>  //Idk
#include <jetgpio.h> //library allowing for pin writing
#include "motor.h"

using namespace motor;

int main()
{
    int Init; // This int will store the jetpio initiallization error code

    Init = gpioInitialise();
    if (Init < 0)
    {
        printf("Jetgpio initialisation failed. Error code %d\n", Init);
    }
    else
    {
        printf("Jetgpio initialisation OK. Return code: %d\n", Init);
    }

    DriveMotor left_Drive_Motor(32);
    DriveMotor right_Drive_Motor(33);
    Actuator top_Actuator(3, 5, 7, 8);
    Actuator bottom_Actuator(11, 13, 15, 16);

    // These are arrays used to split the incoming command
    char arr[100];
    std::string list[10] = {"", "", "", "", "", "", "", "", ""};

    // Used to measure the loop
    char separator = ' ';
    int i;
    int j;
    int topActuatorLimit = 0;
    int bottomAcuatorLimit = 0;

    while (true)
    {
        i = 0;
        j = 0;

        std::cin.getline(arr, 100);
        // Temporary string used to split the string.
        std::string s;

        // This loop splits the incoming command by spaces
        while (arr[i] != '\0')
        {

            if (arr[i] != separator)
            {
                // Append the char to the temp string.
                s += arr[i];
            }
            else
            {
                list[j] = s;
                s.clear();
                j++;
            }
            i++;
        }
        // Store the last word in line.
        list[j] = s;

        if (list[0] == "k")
        {
            left_Drive_Motor.setPercent(0);
            right_Drive_Motor.setPercent(0);
            top_Actuator.stopMovement();
            bottom_Actuator.stopMovement();
        }
        else if (list[0] == "l")
        {
            left_Drive_Motor.setPercent(std::stoi(list[1]));
        }
        else if (list[0] == "r")
        {
            right_Drive_Motor.setPercent(std::stoi(list[1]));
        }
        else if (list[0] == "t")
        {
            if (list[1] == "e")
            { // sets the actuator to extend
                top_Actuator.extend();
            }
            else if (list[1] == "c")
            { // sets the actuator to contract
                top_Actuator.retract();
            }
            else if (list[1] == "s")
            {
                top_Actuator.stopMovement();
            }
        }
        else if (list[0] == "b")
        { // This selects the bottom Actuator
            if (list[1] == "e")
            { // sets the actuator to extend
                bottom_Actuator.extend();
            }
            else if (list[1] == "c")
            { // sets the actuator to contract
                bottom_Actuator.retract();
            }
            else if (list[1] == "s")
            {
                bottom_Actuator.stopMovement();
            }
        }

        if (list[0] == "ping")
        {
            std::cout << "pong" << std::endl;
        }
    }

    // Terminal gpio Library
    gpioTerminate();

    return 0;
}