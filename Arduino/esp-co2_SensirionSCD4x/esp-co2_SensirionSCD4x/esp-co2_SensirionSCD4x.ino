#include <Wire.h>
#include <SensirionI2cScd4x.h>

SensirionI2cScd4x scd4x;

void setup() {
  Serial.begin(115200);

  while (!Serial) {
    delay(10);
  }
  Serial.println("Serial connected.");
  delay(200);

  Wire.begin();
  scd4x.begin(Wire, 0x62);
  scd4x.startPeriodicMeasurement();

  Serial.println("Setup done");
}

void loop() {
  uint16_t co2;
  float temperature, humidity;
  uint16_t error = scd4x.readMeasurement(co2, temperature, humidity);

  if (error == 0 && co2 != 0) {
    Serial.printf("CO2: %i ppm \tTemp: %f°C \tRel.Hum.: %f\%\n", co2, temperature, humidity);

    /*
     * Hier mit Sensorwerten arbeiten
     */
    
    if (co2 < 650) {
      
    }
    else if (co2 < 2000) {
      
    }
    else {
      
    }
  }
}