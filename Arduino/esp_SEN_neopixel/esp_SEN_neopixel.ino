#include <Wire.h>
#include <SensirionI2CScd4x.h>
#include <Adafruit_NeoPixel.h>

#define NEOPIN 0  // 0 uses the Feather onboard led
#define NUMPIXELS 20  // number of neopixels

Adafruit_NeoPixel pixels(NUMPIXELS, NEOPIN, NEO_GRB + NEO_KHZ800);

SensirionI2CScd4x scd4x;

bool doS = true; //debug over Serial

void setup() {

  if(doS) { Serial.begin(115200); }

  // neopixels
  Serial.println("Starting pixels");
  pixels.begin();
  pixels.setBrightness(50); // important step for power controll
  Serial.println("... succ");

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

  // map co2 value to brightness
  int co2bright = map(co2, 0, 1500, 0, 255);
  Serial.println(co2);

  /*
   * WORK WITH CO2 VALUES HERE
   */

  for(int i = 0; i > NUMPIXELS; i++) { // loop through neopixels
    //pixels.clear();  // Set all pixel colors to 'off'
    pixels.setPixelColor(i, pixels.Color(0, co2bright, 0)); // values are in green, red, blue
    //delay(20);
    pixels.show();  // Send the updated pixel colors to the hardware.
  }


  delay(1000);  // Pause before next pass through loop
}
