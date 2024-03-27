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

int main() {
    int Init; // This int will store the jetpio initiallization error code

    Init = gpioInitialise();
    if(Init < 0 ){
        printf("Jetgpio initialisation failed. Error code %d\n", Init);
    }else{
        printf("Jetgpio initialisation OK. Return code: %d\n", Init);
    }


    /* Setting up PWM frequency in Hz @ pin 32 */
    int frequency = 150; // Can be anything from 100 to 200
     gpioSetPWMfrequency(32, frequency);

    int stop_PWM_Value = 0.0015*frequency*256; // 1.5ms/period*256 = 1.5*frequency*256 =
    int num_Partitions = 0.0005*frequency*256;
    gpioPWM(32, stop_PWM_Value); //sets the Pulse length to 1.5ms which is read as a stop on the sparkmax
    
    int x;
    while(true){
        std::cout << "Enter a percentage -100% through 100% at which to run the motor: ";
        std::cin >> x;
        int percent_to_PWM = x*num_Partitions/100-1;
        int PWM_Width = stop_PWM_Value + percent_to_PWM;
        
        gpioPWM(32, PWM_Width);
    }

    // Terminal gpio Library
    gpioTerminate();

    return 0;
}