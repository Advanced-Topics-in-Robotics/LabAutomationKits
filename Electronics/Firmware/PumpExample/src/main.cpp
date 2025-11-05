#include <Arduino.h>
#include "dataModel.h"
#include "df4MotorDriver.h"

int full_speed = 4096; // Full speed for the pumps

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