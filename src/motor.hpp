
#include <iostream>
#include <unistd.h>
#include <jetgpio.h> //library allowing for pin writing
#include <string>
#include <thread>

#define RIGHT_PIN 32            // Left drive motor PWM signal - output pin from jetson
#define LEFT_PIN 33             // Right drive motor PWM signal - output pin from jetson
#define ACTUATOR_1_PIN_A 35     // Back actuator extension signal (a) - output pin from jetson
#define ACTUATOR_1_PIN_B 36     // Back actuator retraction signal (b) - output pin from jetson
#define LIM_SWITCH_1_EXT_PIN 37 // Back actuator extended position limit switch signal (1: stop) - input pin to jetson
#define LIM_SWITCH_1_CON_PIN 38 // Back actuator retracted position limit switch signal (1: stop) - input pin to jetson
#define ACTUATOR_2_PIN_A 28     // Front actuator extension signal (a) - output pin from jetson
#define ACTUATOR_2_PIN_B 29     // Front actuator retration signal (b) - output pin from jetson
#define LIM_SWITCH_2_EXT_PIN 23 // Front actuator extended position limit switch signal (1: stop) - input pin to jetson
#define LIM_SWITCH_2_CON_PIN 26 // Front actuator extended position limit switch signal (1: stop) - input pin to jetson
#define RELAY_PIN 24

int initJetGpio()
{
    int initError = gpioInitialise();

    if (initError < 0)
    {
        printf("Jetgpio initialisation failed. Error code %d\n", initError);
        return initError;
    }
    else
    {
        printf("Jetgpio initialisation OK. Return code: %d\n", initError);
        return 0;
    }
}

class PWMDriveMotor
{
private:
    static const int FREQUENCY = 150;                           // Frequency in Hz to run the PWM at
    static const int STOP_PWM = 0.0015 * FREQUENCY * 256;       // Pulse width of the PWM value to stop the drive motors
    static const int NUM_PARTITIONS = 0.0005 * FREQUENCY * 256; // Difference between STOP_PWM_VALUE and full fowards and full backwards PWM values
    int pwmPinNum;                                              // The pin number controlling the a PWM motor
    int prevPWM = 0;                                            // Previous PWM value

public:
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

    static void runSwitchUpdateLoop(Actuator *actuator)
    {
        while (actuator->isRunning)
        {
            actuator->canExtend = !gpioRead(actuator->switchExtendPin);
            actuator->canRetract = !gpioRead(actuator->switchRetractPin);

            if (!actuator->canExtend || !actuator->canRetract)
            {
                actuator->stopMovement();
            }
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

        // limSwitchThread = std::thread(runSwitchUpdateLoop, this);
    }

    ~Actuator()
    {
        isRunning = false;
        // limSwitchThread.detach();
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
    // 0    0   |   no movement
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

class MotorInterface
{
public:
    // Sets the drive wheel percents [-100, 100] for the left and right wheel
    virtual bool setDrivePercent(int leftPercent, int rightPercent) = 0;

    // Sets the actuator motion for the left and right actuator
    // a | b | motion
    // --|---|-------
    // 0 | 0 | no movement
    // 0 | 1 | retract
    // 1 | 0 | extend
    // 1 | 1 | no movement (prefered not to use)
    virtual bool setActuators(bool a1, bool b1, bool a2, bool b2) = 0;

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
    Actuator actuator1;
    Actuator actuator2;

    bool disableDriveMotors = false;
    bool disableActuators = false;

public:
    MotorController()
        : leftDrive(LEFT_PIN),
          rightDrive(RIGHT_PIN),
          actuator1(ACTUATOR_1_PIN_A, ACTUATOR_1_PIN_B, LIM_SWITCH_1_EXT_PIN, LIM_SWITCH_1_CON_PIN),
          actuator2(ACTUATOR_2_PIN_A, ACTUATOR_2_PIN_B, LIM_SWITCH_2_EXT_PIN, LIM_SWITCH_2_CON_PIN)
    {
        initJetGpio();
    }

    PWMDriveMotor *getLeftDrive()
    {
        return &leftDrive;
    }

    PWMDriveMotor *getRightDrive()
    {
        return &rightDrive;
    }

    Actuator *getActuator1()
    {
        return &actuator1;
    }

    Actuator *getActuator2()
    {
        return &actuator2;
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
    // a | b | motion
    // --|---|-------
    // 0 | 0 | no movement
    // 0 | 1 | retract
    // 1 | 0 | extend
    // 1 | 1 | no movement (prefered not to use)
    bool setActuators(bool a1, bool b1, bool a2, bool b2)
    {
        if (disableActuators)
        {
            return false;
        }
        else
        {
            actuator1.setMotion(a1, b1);
            actuator2.setMotion(a2, b2);
            return true;
        }
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
            setActuators(0, 0, 0, 0);
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

class SimulatedMotorController : public MotorInterface
{
public:
    SimulatedMotorController() {}

    // Sets the drive wheel percents [-100, 100] for the left and right wheel
    bool setDrivePercent(int leftPercent, int rightPercent)
    {
        return false;
    }

    // Sets the actuator motion for the left and right actuator
    // a | b | motion
    // --|---|-------
    // 0 | 0 | no movement
    // 0 | 1 | retract
    // 1 | 0 | extend
    // 1 | 1 | no movement (prefered not to use)
    bool setActuators(bool a1, bool b1, bool a2, bool b2)
    {
        return false;
    }

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

MotorInterface *getMotorContoller()
{
    int error = initJetGpio();

    if (error)
    {
        SimulatedMotorController output;
        return &output;
    }
    else
    {
        MotorController output;
        return &output;
    }
}