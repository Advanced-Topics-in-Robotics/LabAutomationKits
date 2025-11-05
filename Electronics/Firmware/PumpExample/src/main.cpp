#include <Arduino.h>
#include "dataModel.h"
#include "df4MotorDriver.h"

uint16_t full_speed = 4096; // Full speed for the pumps

void setup() {
  Serial.begin(115200);
  Serial.println("Setup started");
  setupPumps();
  Serial.println("Setup finished");
}

void loop() {
  delay(3000);
  Serial.println("Starting pump A forward at full speed for 3 seconds");
  StartPumpA(full_speed, HIGH);

  delay(3000);
  Serial.println("Starting pump A forward at half speed for 3 seconds");
  StartPumpA(full_speed / 2, HIGH);

  delay(3000);
  Serial.println("Starting pump A backward at half speed for 3 seconds");
  StartPumpA(full_speed / 2, LOW);

  delay(3000);
  stopPumps();
  Serial.println("Pumps stopped");
}

void RunPumpA(uint16_t speed, int dir, uint32_t duration_ms) {
    StartPumpA(speed, dir);
    delay(duration_ms);
    StopPumpA();
}

void RunPumpB(uint16_t speed, int dir, uint32_t duration_ms) {
    StartPumpB(speed, dir);
    delay(duration_ms);
    StopPumpB();
}

void RunPumpC(uint16_t speed, int dir, uint32_t duration_ms) {
    StartPumpC(speed, dir);
    delay(duration_ms);
    StopPumpC();
}

void RunAllPums(uint16_t speed, int dir, uint32_t duration_ms) {
    StartPumpA(speed, dir);
    StartPumpB(speed, dir);
    StartPumpC(speed, dir);
    delay(duration_ms);
    StopPumpA();
    StopPumpB();
    StopPumpC();
}