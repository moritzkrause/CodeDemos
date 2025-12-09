#include <SensirionI2cSen66.h>  // @ latest
#include <Wire.h>               // @ latest

#ifdef NO_ERROR
#undef NO_ERROR
#endif
#define NO_ERROR 0

#include <NimBLEDevice.h>  // @ latest
#include "WiFi.h"          // @ latest
#include <HTTPClient.h>    // @ latest

/* 
 * WIFI
 */

String dataurl = "";


/* 
 * Parameters
 */
uint16_t ble_count = 0;
uint16_t rssi_count = 0;

unsigned int lastTime = 0;

/* 
 * Utility Vars
 */
SensirionI2cSen66 sensor;

//static char errorMessage[64];
//static int16_t error;

/* 
 * Values
 *    Variable Name                      MIN   MAX    Precision
 */
float massConcentrationPm1p0 = 0.0;   // 0   | 1000 | ±5% to ±10% m.v.
float massConcentrationPm2p5 = 0.0;   // 0   | 1000 | ±5% to ±10% m.v.
float massConcentrationPm4p0 = 0.0;   // 0   | 500  | ±25% m.v.
float massConcentrationPm10p0 = 0.0;  // 0   | 500  | ±25% m.v.
float temperature = 0.0;              // -10 | 50   | ±0.5 degC m.v.
float humidity = 0.0;                 // 0   | 900  | ±4.5% RH
float voc = 0.0;                   // 1   | 500  | ±15 points (device to device)
float nox = 0.0;                   // 1   | 500  | ±50 points (device to device)
uint16_t co2 = 0;                     // 0   | 40k  | ±50 ppm

int ble_devices = 0;
int rssi_devices = 0;




/*
 * BLE
 */
static constexpr uint32_t scanTimeMs = 1 * 60 * 1000;  // 1 minute scan time.

class scanCallbacks : public NimBLEScanCallbacks {
  void onResult(const NimBLEAdvertisedDevice* advertisedDevice) override {
    ble_count++;
    if (advertisedDevice->getRSSI() >= -60) { // set Threshold
      rssi_count++;
    }
  }

  void onScanEnd(const NimBLEScanResults& results, int reason) override {
    rssi_devices = rssi_count;
    ble_devices = ble_count;
    NimBLEDevice::getScan()->clearResults();
  }
} scanCallbacks;


void setup() {
  delay(2000);

  WiFi.begin("IxDIoT24", "70766086796235198563");
  //WiFi.begin("IxDIoT24", "70766086796235198563");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }


  setupSEN66();
  

  NimBLEDevice::init("");
  NimBLEScan* pBLEScan = NimBLEDevice::getScan();
  pBLEScan->setScanCallbacks(&scanCallbacks, true);
  pBLEScan->setActiveScan(true);
  pBLEScan->setMaxResults(0);

  delay(30000); //Wait 30s for SEN
}

void loop() {
  if ((millis() - lastTime) > scanTimeMs) {

    scanBLE(scanTimeMs/2);
    scanSEN66(true);
    updateSheet();

    lastTime = millis();
  }
}

void scanBLE(uint16_t sT) {
  ble_count = 0;
  rssi_count = 0;
  NimBLEDevice::getScan()->start(sT, false, true);
}


/*
 * SEN66
 */

void setupSEN66() {
  Wire.begin();
  sensor.begin(Wire, SEN66_I2C_ADDR_6B);

  /*error = sensor.deviceReset();
  if (error != NO_ERROR) {
    Serial.print("Error trying to execute deviceReset(): ");
    errorToString(error, errorMessage, sizeof errorMessage);
    Serial.println(errorMessage);
    return;
  } */
  delay(1200);
  /*int8_t serialNumber[32] = { 0 };
  error = sensor.getSerialNumber(serialNumber, 32);
  if (error != NO_ERROR) {
    Serial.print("Error trying to execute getSerialNumber(): ");
    errorToString(error, errorMessage, sizeof errorMessage);
    Serial.println(errorMessage);
    return;
  }
  Serial.print("serialNumber: ");
  Serial.print((const char*)serialNumber);
  Serial.println();
  error = sensor.startContinuousMeasurement();
  if (error != NO_ERROR) {
    Serial.print("Error trying to execute startContinuousMeasurement(): ");
    errorToString(error, errorMessage, sizeof errorMessage);
    Serial.println(errorMessage);
    return;
  } */
  sensor.startContinuousMeasurement();
}
void scanSEN66(bool serial) {
  /*error = sensor.readMeasuredValues(
    massConcentrationPm1p0, massConcentrationPm2p5, massConcentrationPm4p0,
    massConcentrationPm10p0, humidity, temperature, voc, nox,
    co2);*/
  sensor.readMeasuredValues(
    massConcentrationPm1p0, massConcentrationPm2p5, massConcentrationPm4p0,
    massConcentrationPm10p0, humidity, temperature, voc, nox,
    co2);
  /* if (error != NO_ERROR) {
    Serial.print("Error trying to execute readMeasuredValues(): ");
    errorToString(error, errorMessage, sizeof errorMessage);
    Serial.println(errorMessage);
    return;
  } */
  /*if (serial) {
    Serial.print("massConcentrationPm1p0: ");
    Serial.print(massConcentrationPm1p0);
    Serial.print("\t");
    Serial.print("massConcentrationPm2p5: ");
    Serial.print(massConcentrationPm2p5);
    Serial.print("\t");
    Serial.print("massConcentrationPm4p0: ");
    Serial.print(massConcentrationPm4p0);
    Serial.print("\t");
    Serial.print("massConcentrationPm10p0: ");
    Serial.print(massConcentrationPm10p0);
    Serial.print("\t");
    Serial.print("temperature: ");
    Serial.print(temperature);
    Serial.print("\t");
    Serial.print("humidity: ");
    Serial.print(humidity);
    Serial.print("\t");
    Serial.print("vocIndex: ");
    Serial.print(voc);
    Serial.print("\t");
    Serial.print("noxIndex: ");
    Serial.print(nox);
    Serial.print("\t");
    Serial.print("co2: ");
    Serial.print(co2);
    Serial.println();
  }*/
}


/*
 * DATA TRANSFER
 */

void updateSheet() {
  /*
 * STRING FORMATTER ---- SET ROOM !!!!!!!
 */
  // dataurl = "https://script.google.com/macros/s/AKfycbyQRETkDbDOi-ohZq4rhTwPHOBU3_6Q2IyIVtAUCnksIX9JBQGoG1vgUuB_UTtd6c3j/exec?key=CampusMue12346&room=X102";
  // dataurl = "https://script.google.com/macros/s/AKfycbyQRETkDbDOi-ohZq4rhTwPHOBU3_6Q2IyIVtAUCnksIX9JBQGoG1vgUuB_UTtd6c3j/exec?key=CampusMue12346&room=X103";
  // dataurl = "https://script.google.com/macros/s/AKfycbyQRETkDbDOi-ohZq4rhTwPHOBU3_6Q2IyIVtAUCnksIX9JBQGoG1vgUuB_UTtd6c3j/exec?key=CampusMue12346&room=X001";
  dataurl = "https://script.google.com/macros/s/AKfycbyQRETkDbDOi-ohZq4rhTwPHOBU3_6Q2IyIVtAUCnksIX9JBQGoG1vgUuB_UTtd6c3j/exec?key=CampusMue12346&room=X002";
  // dataurl = "https://script.google.com/macros/s/AKfycbyQRETkDbDOi-ohZq4rhTwPHOBU3_6Q2IyIVtAUCnksIX9JBQGoG1vgUuB_UTtd6c3j/exec?key=CampusMue12346&room=XU03";
  dataurl = dataurl + "&ble=" + ble_devices + "&rssi=" + rssi_devices + "&pm1p0=" + massConcentrationPm1p0 + "&pm2p5=" + massConcentrationPm2p5 + "&pm4p0=" + massConcentrationPm4p0 + "&pm10p0=" + massConcentrationPm10p0 + "&temp=" + temperature + "&rh=" + humidity + "&voc=" + voc + "&nox=" + nox + "&co2=" + co2;

  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.setTimeout(5000);
    http.begin(dataurl.c_str());
    byte httpResponseCode = http.GET();

    http.end();
  }

  lastTime = millis();
}