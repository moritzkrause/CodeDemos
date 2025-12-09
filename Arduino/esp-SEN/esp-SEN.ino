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
const char* ssid = "IxDIoT24";
const char* password = "70766086796235198563";

String key = "CampusMue12346";
String dataurl = "";

String serverPath = "https://script.google.com/macros/s/AKfycbyQRETkDbDOi-ohZq4rhTwPHOBU3_6Q2IyIVtAUCnksIX9JBQGoG1vgUuB_UTtd6c3j/exec?";
/* ... = spreadsheet API Endpoint
 * see additional info here: https://developers.google.com/workspace/sheets/api/guides/concepts?hl=de
 * see additional info here: https://script.google.com/macros/s/AKfycbyQRETkDbDOi-ohZq4rhTwPHOBU3_6Q2IyIVtAUCnksIX9JBQGoG1vgUuB_UTtd6c3j/exec?
 */


/* 
 * Parameters
 */
int ble_count = 0;
int rssi_count = 0;
int rssi_thresh = -60;

static const int room = 1;
static const String room_name = "VL";

unsigned long lastTime = 0;
unsigned long startupTimestamp = 0;
unsigned long firstScanTime = 0;
unsigned int startupDelay = 15;  // x 2s delay to ensure PM startup time

/* 
 * Utility Vars
 */
SensirionI2cSen66 sensor;

bool doS = true;

static char errorMessage[64];
static int16_t error;

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
float voc = 0.0;                      // 1   | 500  | ±15 points (device to device)
float nox = 0.0;                      // 1   | 500  | ±50 points (device to device)
uint16_t co2 = 0;                     // 0   | 40k  | ±50 ppm

int ble_devices = 0;
int rssi_devices = 0;




/*
 * BLE
 */
static constexpr uint16_t scanTimeMs = 0.1 * 60 * 1000;  // 1 minute scan time.

class scanCallbacks : public NimBLEScanCallbacks {
  /** Initial discovery, advertisement data only. */
  void onDiscovered(const NimBLEAdvertisedDevice* advertisedDevice) override {
    //Serial.print("Discovered Device: ");
  }

  /**
     *  If active scanning the result here will have the scan response data.
     *  If not active scanning then this will be the same as onDiscovered.
     */
  void onResult(const NimBLEAdvertisedDevice* advertisedDevice) override {
    //Serial.print("Device found! Address: ");
    ble_count++;
    //Serial.println(advertisedDevice->getAddress().toString().c_str());
    if (advertisedDevice->haveManufacturerData()) {
      std::string manufacturerData = advertisedDevice->getManufacturerData();
      // Serial.print("Manufacturer Data: ");
      for (char c : manufacturerData) {
        // Serial.print(c, HEX);
        //Serial.print(c);
        // Serial.print(" ");
      }
    }
    // Serial.println("");
    // Serial.print("RSSI Value: ");
    // Serial.print(advertisedDevice->getRSSI());
    if (advertisedDevice->getRSSI() >= rssi_thresh) {
      // Serial.print(" — added!");
      rssi_count++;
    }
    // Serial.println("\n");
  }

  void onScanEnd(const NimBLEScanResults& results, int reason) override {
    rssi_devices = rssi_count;
    ble_devices = ble_count;
    Serial.println("------ Scan ended. ----- ");
    Serial.printf("Devices: %d\nMeets RSSI: %d\n", ble_devices, rssi_devices);
    NimBLEDevice::getScan()->clearResults();
  }
} scanCallbacks;




void setup() {
  doS = true;  // debug over Serial


  if (doS) Serial.begin(115200);
  while (!Serial && doS) {
    delay(10);
  }
  Serial.println("Serial connected.");
  delay(200);

  setupWIFI();

  setupSEN66();
  startBLE();
  startupTimestamp = millis();

  /*
   * Wait 30s before publishing data to ensure accurate measurements
   */
  for (int i = 0; i >= startupDelay; i++) {
    Serial.printf("Waiting for normalization of values %d/%d", i, startupDelay);
    delay(2000);
  }
  firstScanTime = millis();
}

void loop() {
  if ((millis() - lastTime) > scanTimeMs) {

    scanBLE(scanTimeMs/2); // TODO: scanTimeMS??
    scanSEN66(true);
    updateSheet();

    lastTime = millis();
  }
}


/*
 * WIFI
 */
void setupWIFI() {
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
  Serial.println("WiFi connected!");
}


/*
 * BLE
 */

void startBLE() {
  NimBLEDevice::init("");                             // Initialize the device, you can specify a device name if you want.
  NimBLEScan* pBLEScan = NimBLEDevice::getScan();     // Create the scan object.
  // pBLEScan->setScanCallbacks(&scanCallbacks, false);  // Set the callback for when devices are discovered, no duplicates.
  pBLEScan->setScanCallbacks(&scanCallbacks, true);  // Set the callback for when devices are discovered, no duplicates.
  pBLEScan->setActiveScan(true);                      // Set active scanning, this will get more data from the advertiser.
  pBLEScan->setMaxResults(0);                         // Do not store the scan results, use callback only.
}
void scanBLE(uint16_t sT) {
  ble_count = 0;
  rssi_count = 0;
  Serial.println("---Starting BLE Scan---");
  NimBLEDevice::getScan()->start(sT, false, true);
}


/*
 * SEN66
 */

void setupSEN66() {
  Wire.begin();
  sensor.begin(Wire, SEN66_I2C_ADDR_6B);

  error = sensor.deviceReset();
  if (error != NO_ERROR) {
    Serial.print("Error trying to execute deviceReset(): ");
    errorToString(error, errorMessage, sizeof errorMessage);
    Serial.println(errorMessage);
    return;
  }
  delay(1200);
  int8_t serialNumber[32] = { 0 };
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
  }
}
void scanSEN66(bool serial) {
  error = sensor.readMeasuredValues(
    massConcentrationPm1p0, massConcentrationPm2p5, massConcentrationPm4p0,
    massConcentrationPm10p0, humidity, temperature, voc, nox,
    co2);
  if (error != NO_ERROR) {
    Serial.print("Error trying to execute readMeasuredValues(): ");
    errorToString(error, errorMessage, sizeof errorMessage);
    Serial.println(errorMessage);
    return;
  }
  if (serial) {
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
  }
}


/*
 * STRING FORMATTER
 */
void formatString() {
  dataurl = serverPath + "key=" + key + "&room=" + room_name + "&ble=" + ble_devices + "&rssi=" + rssi_devices + "&timestamp=" + firstScanTime + "&pm1p0=" + massConcentrationPm1p0 + "&pm2p5=" + massConcentrationPm2p5 + "&pm4p0=" + massConcentrationPm4p0 + "&pm10p0=" + massConcentrationPm10p0 + "&temp=" + temperature + "&rh=" + humidity + "&voc=" + voc + "&nox=" + nox + "&co2=" + co2;
  Serial.print("dataurl:\t");
  Serial.println(dataurl);
}



/*
 * DATA TRANSFER
 */

void updateSheet() {
  formatString();
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.setTimeout(5000);
    http.begin(dataurl.c_str());
    int httpResponseCode = http.GET();

    if (httpResponseCode > 0) {
      String payload = http.getString();
    } else {
    }
    http.end();
  }

  lastTime = millis();
}