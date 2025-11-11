#include <Arduino.h>
#include <Wire.h>
#include <ACS71020.h>
#include <SparkFun_ACS37800_Arduino_Library.h>
ACS71020 acs71020;
// #define DEFAULT_VOLTAGE_RANGE 275.0
// #define DEFAULT_CURRENT_RANGE 60.0
const int ACS71020_I2C_ADDRESS = 0x60;
const int sdaPin = 21;
const int sclPin = 22;
void setup() {
  Serial.begin(115200);
  Serial.println("ACS71020 Example");

  Wire.begin(sdaPin, sclPin);

  acs71020.begin(ACS71020_I2C_ADDRESS, Wire);
}

void loop(){
  float voltage = 0.0;
  float current = 0.0;

  ACS71020ERR err = acs71020.readRMS(voltage, current);
  if (err != 0) {
    Serial.print("Error reading RMS values: ");
    Serial.println(err);
    delay(1000);
    return;
  }
  float active_power = 0.0;
  float active_power_avg_sec = 0.0;
  float apparent_power = 0.0;
  float reactive_power = 0.0;
  float power_factor = 0.0;

  acs71020.readPowerActive(active_power);
  Serial.print("Voltage: ");
  Serial.print(voltage, 2);
  Serial.print(" V, Current: ");
  Serial.print(current, 2);
  Serial.println(" A");
  Serial.print("Active Power: ");
  Serial.print(active_power, 2);
  Serial.println(" W");

  delay(1000);
}