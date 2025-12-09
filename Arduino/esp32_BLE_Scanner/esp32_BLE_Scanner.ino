
/* 
 * Parameters
 */
int room = 1;
int ble_devices = 0;
int rssi_devices = 0;
int rssi_thresh = -80;
String name = "X001";
unsigned long lastTime = 0;




/*
 * BLE & WIFI
 */
#include <NimBLEDevice.h>

static constexpr uint32_t scanTimeMs = 10 * 1000;  // 5 seconds scan time.

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
    Serial.print("Device found! Address: ");
    ble_devices++;
    Serial.println(advertisedDevice->getAddress().toString().c_str());
    if (advertisedDevice->haveManufacturerData()) {
      std::string manufacturerData = advertisedDevice->getManufacturerData();
      Serial.print("Manufacturer Data: ");
      for (char c : manufacturerData) {
        Serial.print(c, HEX);
        //Serial.print(c);
        Serial.print(" ");
      }
    }
    Serial.print("RSSI Value: ");
    Serial.print(advertisedDevice->getRSSI());
    if (advertisedDevice->getRSSI() >= rssi_thresh) {
      Serial.print(" — added!");
      rssi_devices++;
    }
    Serial.println(" ");
    Serial.println("\n");
  }

  void onScanEnd(const NimBLEScanResults& results, int reason) override {
    Serial.println("Scan ended.");
    Serial.printf("Devices: %d\nMeets RSSI: %d\n", ble_devices, rssi_devices);
    Serial.println("---Restarting scan---");
    ble_devices = 0;
    rssi_devices = 0;
    NimBLEDevice::getScan()->start(scanTimeMs, false, true);
  }
} scanCallbacks;

/* 
 * WIFI
 */

#include "WiFi.h"
#include <HTTPClient.h>
const char* ssid = "SSID";
const char* password = "password";

String serverPath = "https://script.google.com/macros/s/.../exec?";
/* ... = spreadsheet API Endpoint
 * see additional info here: https://developers.google.com/workspace/sheets/api/guides/concepts?hl=de
 */



void setup() {
  Serial.begin(115200);
  while (!Serial) {
    delay(10);
  }
  Serial.println("Serial connected.");
  delay(200);

  startBLE();
  setupWIFI();
}


void loop() {
  if ((millis() - lastTime) > scanTimeMs) {

    //scanSEN66(true);
    //updateSheet();

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
}

/*
 * BLE
 */

void startBLE() {
  NimBLEDevice::init("");                             // Initialize the device, you can specify a device name if you want.
  NimBLEScan* pBLEScan = NimBLEDevice::getScan();     // Create the scan object.
  pBLEScan->setScanCallbacks(&scanCallbacks, false);  // Set the callback for when devices are discovered, no duplicates.
  pBLEScan->setActiveScan(true);                      // Set active scanning, this will get more data from the advertiser.
  pBLEScan->setMaxResults(0);                         // Do not store the scan results, use callback only.
  pBLEScan->start(scanTimeMs, false, true);           // duration, not a continuation of last scan, restart to get all devices again.
  printf("Scanning...\n");
}


/*
 * STRING FORMATTER
 */
void formatString() {
}



/*
 * DATA TRANSFER
 */

void updateSheet() {
  HTTPClient http;
  http.setTimeout(5000);
  String dataurl = serverPath + "room=" + room + "&devices=" + ble_devices;
  http.begin(dataurl.c_str());
  int httpResponseCode = http.GET();

  if (httpResponseCode > 0) {
    String payload = http.getString();
  } else {
  }
  http.end();

  lastTime = millis();
}
