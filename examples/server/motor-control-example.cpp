//https://github.com/Rubberazer/JETGPIO/blob/main
//
// 1) To run first compile with the following: g++ -Wall -o motorControlTest main.cpp -ljetgpio
//    NOTE: Where motorControlTest is the output file, main.cpp is the file your compiling, and -ljetgpio is the linked file
//
// 2) Now to actually run the program run the following: sudo ./motorControlTest
//

#include <iostream>
#include <unistd.h> //Idk
#include <jetgpio.h> //library allowing for pin writing
#include "Motor.h"

int main() {
    int Init; // This int will store the jetpio initiallization error code

    Init = gpioInitialise();
    if(Init < 0 ){
        printf("Jetgpio initialisation failed. Error code %d\n", Init);
    }else{
        printf("Jetgpio initialisation OK. Return code: %d\n", Init);
    }

    Motor left_Drive_Motor(32);
    Motor right_Drive_Motor(33);
    Motor top_Actuator(3,5,7,8);
    Motor bottom_Actuator(11,13,15,16);
    
    int x;
    while(true){
        std::cout << "Enter a command: ";
        std::cin >> x;
        if( x == 'k'){
            left_Drive_Motor.runDrive(0);
            right_Drive_Motor.runDrive(0);
            top_Actuator.motorStop();
            bottom_Actuator.motorStop();
        }
        left_Drive_Motor.runDrive(x);
        right_Drive_Motor.runDrive(x);
    }

    // Terminal gpio Library
    gpioTerminate();

    return 0;
}