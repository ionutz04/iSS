#include <Arduino.h>
#include <Wire.h>

#define ACS71020_ADDRESS 0x60
#define REG_VIRMS      0x20
#define REG_PACTIVE    0x21


const float vFullScale = 250.0;
const float iFullScale = 30.0;

const int sdaPin = 21;
const int sclPin = 22;
void setup() {
  Serial.begin(115200);
  Wire.begin(sdaPin, sclPin);
}

uint32_t readRegister(uint8_t reg) {
  Wire.beginTransmission(ACS71020_ADDRESS);
  Wire.write(reg);
  Wire.endTransmission(false); // Restart for read
  Wire.requestFrom(ACS71020_ADDRESS, (uint8_t)4);
  uint32_t value = 0;
  if(Wire.available() < 4) {
    Serial.println("Error: Not enough data received from sensor");
    return 0;
  }
  for (int i = 0; i < 4; i++) {
    value <<= 8;
    value |= Wire.read();
  }
  return value;
}

void loop() {
  uint32_t virms = readRegister(REG_VIRMS);
  uint32_t pactive = readRegister(REG_PACTIVE);

  uint16_t vrms_raw = virms & 0xFFFF;          // bits 0–15
  uint16_t irms_raw = (virms >> 16) & 0xFFFF;  // bits 16–31

  // Convert to physical values according to datasheet scaling
  float voltage = (vrms_raw / 32768.0) * vFullScale;
  float current = (irms_raw / 32768.0) * iFullScale;

  // Active power is signed 32-bit with 30 fractional bits (see datasheet!)
  float power = ((int32_t)pactive) / 1073741824.0 * (vFullScale * iFullScale);

  Serial.printf("VRMS: %.2f V, IRMS: %.2f A, Power: %.2f W\n", voltage, current, power);

  delay(500);
}