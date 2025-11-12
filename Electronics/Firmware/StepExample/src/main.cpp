#include <Arduino.h>
#include <CmdMessenger.h>
#include "dataModel.h"
#include "df4MotorDriver.h"

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
  kStepA,              // Command to receive a step (pump state + time), should always contain the full state of the pumps
  kStop,              // Command to stop all pumps
  kStepADone,          // Command to signal a step done

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
  cmdMessenger.attach(kStepA, OnReceiveStep);
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

  cmdMessenger.feedinSerialData();
  loopPump(currentStep.pumpA, kStepADone);
  
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

void receiveStep()
{
  if (getPumpsState() && !getPumpsDone())
  {
    cmdMessenger.readBinArg<bool>();
    cmdMessenger.readBinArg<uint16_t>();
    cmdMessenger.readBinArg<bool>();

    cmdMessenger.readBinArg<unsigned long>();

    cmdMessenger.sendCmd(kError, "Busy soknndosn");
    // cmdMessenger.sendCmdBinArg<unsigned long>(currentStep.time);
  }
  else
  {
    currentStep.pumpA.state = cmdMessenger.readBinArg<bool>();
    currentStep.pumpA.speed = cmdMessenger.readBinArg<uint16_t>();
    currentStep.pumpA.dir = cmdMessenger.readBinArg<bool>();

    pumpA.state = currentStep.pumpA.state;
    pumpA.speed = currentStep.pumpA.speed;
    pumpA.dir = currentStep.pumpA.dir;

    if (currentStep.pumpA.state)
    {
      StartPumpA(currentStep.pumpA.speed, currentStep.pumpA.dir);
    }
    else
    {
      StopPumpA();
    }


    currentStep.pumpA.time = cmdMessenger.readBinArg<unsigned long>();
    currentStep.pumpA.stepStartTime = millis();
    currentStep.pumpA.done = false;

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

