#include <Adafruit_NeoPixel.h>

#define NEOPIN 7  // 0 uses the Feather onboard led
#define NUMPIXELS 60  // number of neopixels

Adafruit_NeoPixel pixels(NUMPIXELS, NEOPIN, NEO_GRB + NEO_KHZ800);

void setup() {

  Serial.begin(115200);

  while (!Serial) {
    delay(10);
  }
  Serial.println("Serial connected.");
  delay(200);

  // neopixels
  Serial.println("Starting pixels and setting brightness 50");
  pixels.begin();
  pixels.show();
  pixels.setBrightness(50); // important step for power controll
  Serial.println("... succ");
}

void loop() {
  
  pixels.clear();

  for(int i = 0; i < NUMPIXELS ; i++) { // loop through neopixels
    for(byte j = 0; j < 120; j+=5) {
      pixels.setPixelColor(i, pixels.Color(0, j, 0)); // values are in green, red, blue
      delay(2);
      pixels.show();
    }
    Serial.print(". ");
    delay(120);
    pixels.show();  // Send the updated pixel colors to the hardware.
  }

  Serial.println("\ndone. \n");
  delay(2000);  // Pause before next pass through loop
}
