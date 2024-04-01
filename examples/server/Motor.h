
#include <iostream>
#include <unistd.h>
#include <jetgpio.h> //library allowing for pin writing
#include <string>

class Motor{
    public:
        //static int frequency; // The frequency that the PWM motors will be run defaults to 150
        //static int stop_PWM; // 1.5ms/period*256 = 1.5*frequency*256 =
        //static int num_Partitions;
        int PWM_Pin_Num; // The pin number controlling the a PWM motor
        
        const int frequency = 150;
        int stop_PWM = 0.0015*frequency*256;
        int num_Partitions = 0.0005*frequency*256;

        // Pins for actuator motors 
        int pinA;
        int pinB;
        int switchA;
        int switchB;

    // This creates a Drive motor
    Motor(int pin){
        PWM_Pin_Num = pin;
        int error_Code = gpioSetPWMfrequency(PWM_Pin_Num, frequency);
        if( error_Code < 0 ){
            printf("Failed to create drive motor obj, Error code: %d\n",error_Code);
        }
    }

    // This creates an actuator motor with limit switches
    Motor(int desired_PinA, int desired_PinB, int desired_LimitA, int desired_LimitB){
        pinA = desired_PinA;
        int pinA_Error = gpioSetMode(pinA, JET_OUTPUT);
        if( pinA_Error < 0 ){
            printf("Failed to create pinA, Error code: %d\n",pinA_Error);
        }

        pinB = desired_PinB;
        int pinB_Error = gpioSetMode(pinB, JET_OUTPUT);
        if( pinB_Error < 0 ){
            printf("Failed to create pinB, Error code: %d\n",pinB_Error);
        }

        switchA = desired_LimitA; // This is the Pin to the full extention limit switch
        int switchA_Error = gpioSetMode(switchA, JET_INPUT);
        if( switchA_Error < 0 ){
            printf("Failed to create limit switchA pin, Error code: %d\n",switchA_Error);
        }

        switchB = desired_LimitB; // This is the Pin to the full retraction limit switch
        int switchB_Error = gpioSetMode(switchA, JET_INPUT);
        if( switchB_Error < 0 ){
            printf("Failed to create limit switchB pin, Error code: %d\n",switchB_Error);
        }
    }

    void runDrive(int power_Percent){ // run drive motor
        int percent_to_PWM = power_Percent*num_Partitions/100;
        int PWM_Width = stop_PWM + percent_to_PWM;
        gpioPWM(PWM_Pin_Num, PWM_Width);
    }

    void motorExtend(){
        if(switchA != 1){
            gpioWrite(pinA, 0);
            gpioWrite(pinB, 1);
        }
    }

    void motorContract(){
        if(switchB != 1){
            gpioWrite(pinA, 1);
            gpioWrite(pinB, 0);
        }
    }

    void motorStop(){
        gpioWrite(pinA, 0);
        getw(pinB, 0);
    }
};