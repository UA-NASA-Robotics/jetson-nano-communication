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
#include "motor.hpp"

int main()
{
    MotorController motors;

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
            motors.stopMovement();
        }
        else if (list[0] == "l")
        {
            motors.getLeftDrive()->setPercent(std::stoi(list[1]));
        }
        else if (list[0] == "r")
        {
            motors.getRightDrive()->setPercent(std::stoi(list[1]));
        }
        else if (list[0] == "t")
        {
            if (list[1] == "e")
            { // sets the actuator to extend
                motors.getActuator1()->extend();
            }
            else if (list[1] == "c")
            { // sets the actuator to contract
                motors.getActuator1()->retract();
            }
            else if (list[1] == "s")
            {
                motors.getActuator1()->stopMovement();
            }
        }
        else if (list[0] == "b")
        { // This selects the bottom Actuator
            if (list[1] == "e")
            { // sets the actuator to extend
                motors.getActuator2()->extend();
            }
            else if (list[1] == "c")
            { // sets the actuator to contract
                motors.getActuator2()->retract();
            }
            else if (list[1] == "s")
            {
                motors.getActuator2()->stopMovement();
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