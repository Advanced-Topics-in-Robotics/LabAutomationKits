#include <Arduino.h>
#include <CmdMessenger.h>
#include "dataModel.h"
#include "df4MotorDriver.h"
#include <SparkFun_AS7343.h>

SfeAS7343ArdI2C mySensor;

uint16_t myData[ksfAS7343NumChannels]; // Array to hold spectral data

CmdMessenger cmdMessenger(Serial, ',', ';', '/');

// This is the list of recognized commands. These can be commands that can either be sent or received.
// In order to receive, attach a callback function to these events
void OnUnknownCommand();
void OnWatchdogRequest();
void OnArduinoReady();
void OnGetState();
void OnGetLastStep();
void OnReceiveStep();
void OnReceiveStop();

void returnState();
void returnLastStep();
void receiveStep();
void receiveStop();

enum Commands
{
  // Commands
  kWatchdog,          // Command to request application ID
  kAcknowledge,       // Command to acknowledge a received command
  kError,             // Command to message that an error has occurred
  kGetState,          // Command to get the pump states
  kGetStateResult,    // Command to send the full state of the pumps
  kGetLastStep,       // Command to get the current step
  kGetLastStepResult, // Command to send the current step
  kStep,              // Command to receive a step (pump state + time), should always contain the full state of the pumps
  kStop,              // Command to stop all pumps
  kStepADone,          // Command to signal a step done
  kStepBDone,          // Command to signal a step done
  kStepCDone           // Command to signal a step done
};

// Commands we send from the PC and want to receive on the Arduino.
// We must define a callback function in our Arduino program for each entry in the list below.
void attachCommandCallbacks()
{
  // Attach callback methods
  cmdMessenger.attach(OnUnknownCommand);
  cmdMessenger.attach(kWatchdog, OnWatchdogRequest);
  cmdMessenger.attach(kGetState, OnGetState);
  cmdMessenger.attach(kGetLastStep, OnGetLastStep);
  cmdMessenger.attach(kStep, OnReceiveStep);
  cmdMessenger.attach(kStop, OnReceiveStop);
}

// ------------------  C A L L B A C K S -----------------------

// Called when a received command has no attached function
void OnUnknownCommand()
{
  cmdMessenger.sendCmd(kError, "Command without attached callback");
}

void OnWatchdogRequest()
{
  // Will respond with same command ID and Unique device identifier.
  cmdMessenger.sendCmd(kWatchdog, "0000000-0000-0000-0000-00000000001");
}

// Callback function that responds that Arduino is ready (has booted up)
void OnArduinoReady()
{
  cmdMessenger.sendCmd(kAcknowledge, "Arduino ready");
}

void OnGetState()
{
  returnState();
}

void OnGetLastStep()
{
  returnLastStep();
}

void OnReceiveStep()
{
  receiveStep();
}

void OnReceiveStop()
{
  receiveStop();
}

void setup() {

  Serial.begin(115200);
  setupPumps();
  stopPumps();

   Wire.begin();
   
   // Initialize sensor and run default setup.
    if (mySensor.begin() == false)
    {
        Serial.println("Sensor failed to begin. Please check your wiring!");
        Serial.println("Halting...");
        while (1)
            ;
    }

        // Power on the device
    if (mySensor.powerOn() == false)
    {
        Serial.println("Failed to power on the device.");
        Serial.println("Halting...");
        while (1)
            ;
    }

    // Set the AutoSmux to output all 18 channels
    if (mySensor.setAutoSmux(AUTOSMUX_18_CHANNELS) == false)
    {
        Serial.println("Failed to set AutoSmux.");
        Serial.println("Halting...");
        while (1)
            ;
    }

    // Enable Spectral Measurement
    if (mySensor.enableSpectralMeasurement() == false)
    {
        Serial.println("Failed to enable spectral measurement.");
        Serial.println("Halting...");
        while (1)
            ;
    }
  //  Do not print newLine at end of command,
  //  in order to reduce data being sent
  cmdMessenger.printLfCr(false);

  // Attach my application's user-defined callback methods
  attachCommandCallbacks();

  cmdMessenger.sendCmd(kAcknowledge, "Arduino has started!");
 
}

void loopPump(Pump &pump, Commands stepDoneCmd)
{
  if (pump.state){
    if (!pump.done){
      if (millis() - pump.stepStartTime >= pump.time){
        pump.done = true;
        stopPump();
        cmdMessenger.sendCmd(stepDoneCmd);
      }
    } else {
      pump.state = false;
    }
  }
}

void loop() {

    mySensor.ledOn();
    delay(1000);
    
    mySensor.ledOff();
    delay(1000);
  cmdMessenger.feedinSerialData();
  loopPump(currentStep.pumpA, kStepADone);
  loopPump(currentStep.pumpB, kStepBDone);
  loopPump(currentStep.pumpC, kStepCDone);
}


void returnState()
{
  cmdMessenger.sendCmdStart(kGetStateResult);

  cmdMessenger.sendCmdBinArg<bool>(pumpA.state);
  cmdMessenger.sendCmdBinArg<uint16_t>(pumpA.speed);
  cmdMessenger.sendCmdBinArg<bool>(pumpA.dir);

  cmdMessenger.sendCmdEnd();
}

void returnLastStep()
{

  cmdMessenger.sendCmdStart(kGetLastStepResult);

  cmdMessenger.sendCmdBinArg<bool>(getPumpsState());
  cmdMessenger.sendCmdBinArg<bool>(getPumpsDone());
  cmdMessenger.sendCmdBinArg<unsigned long>(currentStep.pumpA.time);

  cmdMessenger.sendCmdBinArg<bool>(currentStep.pumpA.state);
  cmdMessenger.sendCmdBinArg<uint16_t>(currentStep.pumpA.speed);
  cmdMessenger.sendCmdBinArg<bool>(currentStep.pumpA.dir);

  cmdMessenger.sendCmdEnd();
}

void readPumpStep(Pump &currentStepPump, Pump &pump)
{
    currentStepPump.state = cmdMessenger.readBinArg<bool>();
    currentStepPump.speed = cmdMessenger.readBinArg<uint16_t>();
    currentStepPump.dir = cmdMessenger.readBinArg<bool>();

    currentStepPump.time = cmdMessenger.readBinArg<unsigned long>();
    currentStepPump.stepStartTime = millis();
    currentStepPump.done = false; 

    pump.state = currentStepPump.state;
    pump.speed = currentStepPump.speed;
    pump.dir = currentStepPump.dir;
}

void receiveStep()
{
  if (getPumpsState() && !getPumpsDone())
  {
    cmdMessenger.readBinArg<bool>();
    cmdMessenger.readBinArg<uint16_t>();
    cmdMessenger.readBinArg<bool>();

    cmdMessenger.readBinArg<unsigned long>();

    cmdMessenger.readBinArg<bool>();
    cmdMessenger.readBinArg<uint16_t>();
    cmdMessenger.readBinArg<bool>();

    cmdMessenger.readBinArg<unsigned long>();

    cmdMessenger.readBinArg<bool>();
    cmdMessenger.readBinArg<uint16_t>();
    cmdMessenger.readBinArg<bool>();

    cmdMessenger.readBinArg<unsigned long>();

    cmdMessenger.sendCmd(kError, "Busy");
    // cmdMessenger.sendCmdBinArg<unsigned long>(currentStep.time);
  }
  else
  {
    readPumpStep(currentStep.pumpA, pumpA);
    readPumpStep(currentStep.pumpB, pumpB);
    readPumpStep(currentStep.pumpC, pumpC);


    if (currentStep.pumpA.state)
    {
      StartPumpA(currentStep.pumpA.speed, currentStep.pumpA.dir);
    }
    else
    {
      StopPumpA();
    }

    if (currentStep.pumpB.state)
    {
      StartPumpB(currentStep.pumpB.speed, currentStep.pumpB.dir);
    }
    else
    {
      StopPumpB();
    }

    if (currentStep.pumpC.state)
    {
      StartPumpC(currentStep.pumpC.speed, currentStep.pumpC.dir);
    }
    else
    {
      StopPumpC();
    }


    cmdMessenger.sendCmdStart(kAcknowledge);
    cmdMessenger.sendCmdArg("Step");
    // cmdMessenger.sendCmdBinArg<unsigned long>(currentStep.time);
    cmdMessenger.sendCmdEnd();
  }
}

void receiveStop()
{

  stopPumps();

  setDonePumps(true);
  setStatePumps(false);
  cmdMessenger.sendCmd(kAcknowledge, "Stopped");
}

