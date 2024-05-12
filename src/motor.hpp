#pragma once

#include <iostream>
#include <unistd.h>
#include <jetgpio.h> //library allowing for pin writing
#include <string>
#include <thread>
#include "types.hpp"
#include "serial/serial.h"
#include "arduino.hpp"
#include <chrono>

#define NUM_ACTUATORS 2

#define RIGHT_PIN 32            // Left drive motor PWM signal - output pin from jetson
#define LEFT_PIN 33             // Right drive motor PWM signal - output pin from jetson
#define ACTUATOR_1_PIN_A 35     // Back actuator extension signal (a) - output pin from jetson
#define ACTUATOR_1_PIN_B 36     // Back actuator retraction signal (b) - output pin from jetson
#define ACTUATOR_2_PIN_A 28     // Front actuator extension signal (a) - output pin from jetson
#define ACTUATOR_2_PIN_B 29     // Front actuator retration signal (b) - output pin from jetson
#define PORT_ARD "/dev/ttyACM0"
#define PITCH_IN 0
#define PITCH_OUT 90
#define ARM_IN 0
#define ARM_OUT 90

class PWMDriveMotor
{
private:
    static const int FREQUENCY = 150;                           // Frequency in Hz to run the PWM at
    static const int STOP_PWM = 0.0015 * FREQUENCY * 256;       // Pulse width of the PWM value to stop the drive motors
    static const int NUM_PARTITIONS = 0.001 * FREQUENCY * 256; // Difference between STOP_PWM_VALUE and full fowards and full backwards PWM values
    int pwmPinNum;                                              // The pin number controlling the a PWM motor
    int prevPercent = 0;                                        // Previous PWM value

public:
    PWMDriveMotor() {}
    PWMDriveMotor(int pin)
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

    ~PWMDriveMotor()
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
        // Flip percent becuase of way physical motorrs are oriented
        percent = -percent;
        // Check if percent is outside of [-100, 100]
        if (percent > 100)
        {
            percent = 100;
        }
        else if (percent < -100)
        {
            percent = -100;
        }

        // If the percent equals the previous one, then return to reduce stuttering
        if (percent == prevPercent)
        {
            return;
        }

        prevPercent = percent;

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
    double insideAngle;   // Angle of inside limit
    double outsideAngle;  // Angle of outside limit

    int prevA = 0; // Previous pin A value
    int prevB = 0; // Previous pin B value

    bool canExtend = true;  // Read value from the limit switch extension pin
    bool canRetract = true; // Read value from the limit switch retraction pin

public:
    Actuator() {}
    Actuator(int desiredPinA, int desiredPinB)
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
    }

    ~Actuator() {}

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
        gpioWrite(pinA, 1);
        gpioWrite(pinB, 1);
    }

    double* getAngles()
    {
        return new double[2] {insideAngle, outsideAngle};
    }

    // Set the motion for the actuator, returns true if it can, false if it cannot
    // a    b   |   motion
    // ------------------------
    // 0    0   |   no movement
    // 0    1   |   contracting
    // 1    0   |   extending
    // 1    1   |   no movement -- try not to do this one
    // This is what will be called when using the boolean input to decide movement
    bool setMotion(ActuatorMotion motion)
    {
        if (motion == ActuatorMotion::EXTENDING)
        {
            return extend();
        }
        else if (motion == ActuatorMotion::RETRACTING)
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

class MotorInterface
{
public:
    MotorInterface() {}
    ~MotorInterface() {}

    // serial::Serial arduino;

    // Sets the drive wheel percents [-100, 100] for the left and right wheel
    virtual bool setDrivePercent(int leftPercent, int rightPercent) = 0;

    // Sets the actuator motion for the left and right actuator
    virtual bool setActuators(ActuatorMotion a1, ActuatorMotion a2) = 0;

    virtual void setActuatorPercent(double *bytes) = 0;

    virtual bool stopMovement() = 0;

    // Cancels currently executing macro if any is executing, returns true if one was running, false if none was
    virtual bool cancelMacro() = 0;

    // Macro to execute a full dig cycle and disable manual actuator control in the meantime
    // Returns true if successfully started, false if not
    virtual bool digCycle() = 0;

    // Macro to execute a full dump cycle and disable manual actuator control in the meantime
    // Returns true if successfully started, false if not
    virtual bool dumpCycle() = 0;

    // Macro to turn the robot <angle> degrees, -180 <= angle <= 180
    // Returns true if successfully started, false if not
    virtual bool turn(int angle) = 0;
};

class MotorController : public MotorInterface
{
private:
    PWMDriveMotor leftDrive;
    PWMDriveMotor rightDrive;
    Actuator actuators[NUM_ACTUATORS];
    Arduino arduino;

    bool disableDriveMotors = false;
    bool disableActuators = false;

public:
    MotorController()
    {
        leftDrive = PWMDriveMotor(LEFT_PIN);
        rightDrive = PWMDriveMotor(RIGHT_PIN);
        actuators[0].~Actuator();
        actuators[1].~Actuator();
        actuators[0] = Actuator(ACTUATOR_1_PIN_A, ACTUATOR_1_PIN_B);
        actuators[1] = Actuator(ACTUATOR_2_PIN_A, ACTUATOR_2_PIN_B);
        arduino = Arduino("/dev/ttyACM0", 115200);
    }

    ~MotorController()
    {
        gpioTerminate();
    }

    PWMDriveMotor *getLeftDrive()
    {
        return &leftDrive;
    }

    PWMDriveMotor *getRightDrive()
    {
        return &rightDrive;
    }

    Actuator *getActuator(int index)
    {
        if (index >= 0 && index < NUM_ACTUATORS)
        {
            return &actuators[index];
        }
        return nullptr;
    }

    Actuator *getActuator1()
    {
        return &actuators[0];
    }

    Actuator *getActuator2()
    {
        return &actuators[1];
    }

    // Sets the drive wheel percents [-100, 100] for the left and right wheel
    bool setDrivePercent(int leftPercent, int rightPercent)
    {
        if (disableDriveMotors)
        {
            return false;
        }
        else
        {
            leftDrive.setPercent(leftPercent);
            rightDrive.setPercent(rightPercent);
            return true;
        }
    }

    // Sets the actuator motion for the left and right actuator
    bool setActuators(ActuatorMotion a1, ActuatorMotion a2)
    {
        if (disableActuators)
        {
            return false;
        }
        else
        {
            actuators[0].setMotion(a1);
            actuators[1].setMotion(a2);
            return true;
        }
    }

    bool setActuators(ActuatorMotion motions[NUM_ACTUATORS])
    {
        if (disableActuators)
        {
            return false;
        }
        else
        {
            bool success = true;
            for (int i = 0; i < NUM_ACTUATORS; i++)
            {
                if (!actuators[i].setMotion(motions[i]))
                {
                    success = false;
                }
            }
            return success;
        }
    }

    void setActuatorPercent(double *percents)
    {
	disableActuators = true;
        double differences[2] = {PITCH_OUT - PITCH_IN, ARM_OUT - ARM_IN};
        double *goals = percents;

        bool extending[2] = {0, 0};
        bool finishedMoving[2] = {0, 0};
        double dataPoints[2] = {0, 0};

        arduino.flushBuffer();
        arduino.readFilterdData(dataPoints);

        extending[0] = dataPoints[0] < goals[0];
        extending[1] = dataPoints[1] < goals[1];

        while (!finishedMoving[0] || !finishedMoving[1]) {
            arduino.readFilterdData(dataPoints);

            for (int i = 0; i < 2; i++) {
                if (dataPoints[i] > goals[i] && extending[i] || dataPoints[i] < goals[i] && !extending[i]) {
                    finishedMoving[i] = true;
                }

                if (!finishedMoving[i]) {
                    if (extending[i]) {
                        getActuator(i)->extend();
                    } else {
                        getActuator(i)->retract();
                    }
                } else {
                    getActuator(i)->stopMovement();
                }
            }
        }
	disableActuators = false;
    }

    bool stopMovement()
    {
        if (disableDriveMotors)
        {
            return false;
        }
        else
        {
            setDrivePercent(0, 0);
        }

        if (disableActuators)
        {
            return false;
        }
        else
        {
            setActuators(ActuatorMotion::NONE, ActuatorMotion::NONE);
        }

        return true;
    }

    // Cancels currently executing macro if any is executing, returns true if one was running, false if none was
    bool cancelMacro()
    {
        // TODO: implement functionality to cancel currently executing macro
        return false;
    }

    // Macro to execute a full dig cycle and disable manual actuator control in the meantime
    // Returns true if successfully started, false if not
    bool digCycle()
    {
	try {
	    std::cout << "Executing Dig Cycle" << std::endl;
            setActuatorPercent(new double[2] {3, 7});
	    setDrivePercent(25, 25);
	    std::this_thread::sleep_for(std::chrono::seconds(8));
	    setDrivePercent(0, 0);
	    setActuatorPercent(new double[2] {-20, -51});
	    return true;
	} catch (...) {
	    std::cout << "Error" << std::endl;
	    return false;
	}
    }

    // Macro to execute a full dump cycle and disable manual actuator control in the meantime
    // Returns true if successfully started, false if not
    bool dumpCycle()
    {
	try {
            std::cout << "Executing Dump Cycle" << std::endl;
	    setDrivePercent(25, 25);
	    std::this_thread::sleep_for(std::chrono::seconds(2));
	    setDrivePercent(-20, -51);
            setActuatorPercent(new double[2] {23, -10});
	    setDrivePercent(-25, -25);
	    std::this_thread::sleep_for(std::chrono::seconds(4));
	    setDrivePercent(-20, -51);
	    return true;
	} catch (...) {
	    return false;
	}
    }

    // Macro to turn the robot <angle> degrees, -180 <= angle <= 180
    // Returns true if successfully started, false if not
    bool turn(int angle)
    {
        // TODO: implement functionality to turn the robot by <angle> degrees
        return false;
    }
};

class SimulatedMotorController : public MotorInterface
{
public:
    SimulatedMotorController() {}
    ~SimulatedMotorController() {}

    // Sets the drive wheel percents [-100, 100] for the left and right wheel
    bool setDrivePercent(int leftPercent, int rightPercent)
    {
        return false;
    }

    // Sets the actuator motion for the left and right actuator
    bool setActuators(ActuatorMotion a1, ActuatorMotion a2)
    {
        return false;
    }

    void setActuatorPercent(double *bytes) {}

    bool stopMovement()
    {
        return false;
    }

    // Cancels currently executing macro if any is executing, returns true if one was running, false if none was
    bool cancelMacro()
    {
        // TODO: implement functionality to cancel currently executing macro
        return false;
    }

    // Macro to execute a full dig cycle and disable manual actuator control in the meantime
    // Returns true if successfully started, false if not
    bool digCycle()
    {
        // TODO: implement dig cycle to be executed in another thread
        return false;
    }

    // Macro to execute a full dump cycle and disable manual actuator control in the meantime
    // Returns true if successfully started, false if not
    bool dumpCycle()
    {
        // TODO: implement dump cycle to be executed in another thread
        return false;
    }

    // Macro to turn the robot <angle> degrees, -180 <= angle <= 180
    // Returns true if successfully started, false if not
    bool turn(int angle)
    {
        // TODO: implement functionality to turn the robot by <angle> degrees
        return false;
    }
};

int initJetGpio()
{
    int initError = gpioInitialise();

    if (initError != 1)
    {
        printf("Jetgpio initialisation failed. Error code %d\n", initError);
    }
    else
    {
        printf("Jetgpio initialisation OK. Return code: %d\n", initError);
    }

    return initError;
}

MotorInterface *getMotorContoller()
{
    int error = initJetGpio();

    if (error != 1)
    {
        return new SimulatedMotorController();
    }
    else
    {
        return new MotorController();
    }
}
