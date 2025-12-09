// Board: M5Atom
// Libraries: M5Atom by M5Stack, NimBLED

#include <NimBLEDevice.h>
#include "M5Atom.h"

int scanTime = 5; //In seconds
BLEScan* pBLEScan;

class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice* advertisedDevice) {
      //Serial.printf("Advertised Device: %s \n", advertisedDevice->toString().c_str());
    }
};

void setup() {
  Serial.begin(115200);
  while(!Serial) {
    delay(10);
  }
  
  Serial.println("Scanning...");

  BLEDevice::init("");
  pBLEScan = BLEDevice::getScan(); //create new scan
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true); //active scan uses more power, but get results faster
  pBLEScan->setInterval(100);
  pBLEScan->setWindow(99);  // less or equal setInterval value

  // M5Atom
  M5.begin(true, false, true);
  delay(50);
  M5.dis.begin(5*5);
  M5.dis.drawpix(0, 0x00ff00); // green
}

void loop() {
  // put your main code here, to run repeatedly:
  BLEScanResults foundDevices = pBLEScan->start(scanTime, false);
  Serial.print("Devices found: ");
  Serial.println(foundDevices.getCount());
  Serial.println("Scan done!");

  M5.dis.clear();
  int devices = foundDevices.getCount(); // number of ble devices
  for (int i=0; i<devices; i++) {
    if (i < devices) {
      M5.dis.drawpix(i, 0x8FFF00); // green leds
    }
    if (i < devices && i >= 25) {
      M5.dis.drawpix(i-25, 0xE75517); // red leds
    }
    if (i < devices && i >= 50) {
      M5.dis.drawpix(i-50, 0x5414F9); // blue leds
    }
    if (i < devices && i >= 75) {
      M5.dis.drawpix(i-75, 0xFF28B4); // pink leds
    }
    if (i < devices && i >= 100) {
      M5.dis.drawpix(i-100, 0xeeee22); // yellow leds
    }
    if (i < devices && i >= 125) {
      M5.dis.drawpix(i-125, 0x22eeb4); // aqua leds
    }
    if (i < devices && i >= 150) {
      M5.dis.drawpix(i-150, 0xffffff); // white leds
    }
  }

  pBLEScan->clearResults();   // delete results fromBLEScan buffer to release memory
  delay(2000);

  M5.update();
}