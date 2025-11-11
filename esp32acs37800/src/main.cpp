
#include "SparkFun_ACS37800_Arduino_Library.h" 
#include <Wire.h>

ACS37800 mySensor; //Create an object of the ACS37800 class

void setup()
{
  Serial.begin(115200);
  Serial.println(F("ACS37800 Power Meter"));
  Wire.begin();
 
  //mySensor.enableDebugging(); // Uncomment this line to print useful debug messages to Serial
  //Initialize sensor using default I2C address

  if (mySensor.begin(0x61) == false)
  {
    Serial.print(F("ACS37800 not detected. Check connections and I2C address. Freezing..."));
    while (1);
  }
      mySensor.setBypassNenable(false, true); // Disable bypass_n in shadow memory and eeprom

  mySensor.setDividerRes(3925000);
  mySensor.setSenseRes(1830);
  mySensor.setCurrentRange(30);
  mySensor.setI2Caddress(0x61);
}

void loop()
{
  float volts = 0.0;        float amps = 0.0;
  float pactive = 0.0;      float preactive = 0.0;
  float papparent = 0.0;    float pfactor = 0.0;
  bool posangle = 0;        bool pospf = 0;

    mySensor.readRMS(&volts, &amps); // Read the RMS voltage and current
    mySensor.readPowerActiveReactive(&pactive, &preactive); // Read the active and reactive power
    mySensor.readPowerFactor(&papparent, &pfactor, &posangle, &pospf); // Read the apparent power and the power factor

    Serial.print(F("\nVoltage [V]: "));
    Serial.print(volts, 3);
    Serial.print(F("         Current [A]: "));
    Serial.println(amps, 3);
    Serial.print(F("Active Power [W]: "));
    Serial.print(pactive, 3);
    Serial.print(F("      Reactive Power [VAR]: "));
    Serial.println(preactive, 3);
    Serial.print(F("Apparent Power [VA]: "));
    Serial.print(papparent, 3);
    Serial.print(F("  Power Factor: "));
    Serial.print(pfactor, 3);
    Serial.print("\n");

  /*if (posangle)
    Serial.print(F("    Lagging"));
  else
    Serial.print(F("    Leading"));
  if (pospf)
    Serial.println(F("    Consumed"));
  else
    Serial.println(F("    Generated"));*/
  
  delay(1000);
}
