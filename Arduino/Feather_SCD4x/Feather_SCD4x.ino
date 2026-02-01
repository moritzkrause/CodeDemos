#include <SensirionI2CScd4x.h>

SensirionI2CScd4x scd4x;

bool doS = true; //debug over Serial

void setup() {

  if(doS) { Serial.begin(115200); }

  // sensirion
  Serial.println("\nStarting co2");
  Wire.begin();
  scd4x.begin(Wire);
  uint16_t error = scd4x.stopPeriodicMeasurement();
  delay(100);
  error = scd4x.startPeriodicMeasurement();
  Serial.println("... succ");
}

void loop() {

  // read measurements and store them in variables
  uint16_t co2;
  float temperature;
  float humidity;
  uint16_t error = scd4x.readMeasurement(co2, temperature, humidity);

  // map co2 value to custom value, eg 255 for pixels
  int co2bright = map(co2, 0, 1500, 0, 255);
  Serial.println(co2);

  /*
   * WORK WITH CO2 VALUES HERE
   */


  delay(1000);  // Pause before next pass through loop
}
