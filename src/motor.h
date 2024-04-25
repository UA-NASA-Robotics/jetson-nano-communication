
#include <iostream>
#include <unistd.h>
#include <jetgpio.h> //library allowing for pin writing
#include <string>
#include <thread>

const int FREQUENCY = 150;                           // Frequency in Hz to run the PWM at
const int STOP_PWM = 0.0015 * FREQUENCY * 256;       // Pulse width of the PWM value to stop the drive motors
const int NUM_PARTITIONS = 0.0005 * FREQUENCY * 256; // Difference between STOP_PWM_VALUE and full fowards and full backwards PWM values

namespace motor
{
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
        return 0;
    }

    class DriveMotor
    {
    public:
        int pwmPinNum;   // The pin number controlling the a PWM motor
        int prevPWM = 0; // Previous PWM value

        DriveMotor(int pin)
        {
            pwmPinNum = pin;

            int errorCode = gpioSetPWMfrequency(pwmPinNum, FREQUENCY);
            if (errorCode < 0)
            {
                printf("Failed to create drive motor obj, Error code: %d\n", errorCode);
                return;
            }

            errorCode = gpioPWM(pwmPinNum, STOP_PWM);
            if (errorCode < 0)
            {
                printf("Failed to set drive motor PWM, Error code: %d\n", errorCode);
                return;
            }
        }

        ~DriveMotor()
        {
            int errorCode = gpioPWM(pwmPinNum, STOP_PWM);
            if (errorCode < 0)
            {
                printf("Failed to set drive motor PWM, Error code: %d\n", errorCode);
                return;
            }
        }

        void setPercent(int percent)
        {
            int percentToPWM = percent * NUM_PARTITIONS / 100;
            int dutyCycle = STOP_PWM + percentToPWM;
            int errorCode = gpioPWM(pwmPinNum, dutyCycle);
            if (errorCode < 0)
            {
                printf("Failed to set drive motor PWM, Error code: %d\n", errorCode);
                return;
            }
        }
    };

    class Actuator
    {
    private:
        int pinA;             // Extension signal logic pin output for actuator
        int pinB;             // Retration signal logic pin output for actuator
        int switchExtendPin;  // Limit switch extended logic pin input
        int switchRetractPin; // Limit switch retrated logic pin input

        int prevA = 0; // Previous pin A value
        int prevB = 0; // Previous pin B value

        bool canExtend = false;  // Read value from the limit switch extension pin
        bool canRetract = false; // Read value from the limit switch retraction pin

        bool isRunning = true;

        std::thread limSwitchThread;

        void runSwitchUpdateLoop()
        {
            while (isRunning)
            {
                canExtend = !gpioRead(switchExtendPin);
                canRetract = !gpioRead(switchRetractPin);
            }
        }

    public:
        Actuator(int desiredPinA, int desiredPinB, int desiredLimitA, int desiredLimitB)
        {
            pinA = desiredPinA;
            int pinA_Error = gpioSetMode(pinA, JET_OUTPUT);
            if (pinA_Error < 0)
            {
                printf("Failed to create pinA, Error code: %d\n", pinA_Error);
            }

            pinB = desiredPinB;
            int pinB_Error = gpioSetMode(pinB, JET_OUTPUT);
            if (pinB_Error < 0)
            {
                printf("Failed to create pinB, Error code: %d\n", pinB_Error);
            }

            switchExtendPin = desiredLimitA; // This is the Pin to the full extention limit switch
            int switchA_Error = gpioSetMode(switchExtendPin, JET_INPUT);
            if (switchA_Error < 0)
            {
                printf("Failed to create limit switchA pin, Error code: %d\n", switchA_Error);
            }

            switchRetractPin = desiredLimitB; // This is the Pin to the full retraction limit switch
            int switchB_Error = gpioSetMode(switchRetractPin, JET_INPUT);
            if (switchB_Error < 0)
            {
                printf("Failed to create limit switchB pin, Error code: %d\n", switchB_Error);
            }

            limSwitchThread = std::thread(runSwitchUpdateLoop);
        }

        ~Actuator()
        {
            isRunning = false;
        }

        // Set the actuator to extend, returns true if it can, false if it cannot
        bool extend()
        {
            if (canExtend)
            {
                gpioWrite(pinA, 0);
                gpioWrite(pinB, 1);
                return true;
            }
            else
            {
                return false;
            }
        }

        // Set the actuator to retract, returns true if it can, false if it cannot
        bool retract()
        {
            if (canRetract)
            {
                gpioWrite(pinA, 1);
                gpioWrite(pinB, 0);
                return true;
            }
            else
            {
                return false;
            }
        }

        void stopMovement()
        {
            gpioWrite(pinA, 0);
            gpioWrite(pinB, 0);
        }

        // Set the motion for the actuator, returns true if it can, false if it cannot
        // a    b   |   motion
        // ------------------------
        // 0    0   |   no movement !gpioRead(LIM_SWITCH_1_EXT_PIN)
        // 0    1   |   contracting
        // 1    0   |   extending
        // 1    1   |   no movement -- try not to do this one
        // This is what will be called when using the boolean input to decide movement
        bool setMotion(bool a, bool b)
        {
            if (a && !b)
            {
                return extend();
            }
            else if (!a && b)
            {
                return retract();
            }
            else
            {
                stopMovement();
                return true;
            }
        }
    };
}