
/* written for Adafruit Feather ESP32 C6 
 * Jan 2026
 */

#include <WiFi.h>         // by ESP Board @ current
#include <HTTPClient.h>   // by Arduino @ 0.6.1
#include <ArduinoJson.h>  // by Arduino @0.2.0

#include <CSV_Parser.h> // by Michal Borowski @ 1.4.1
//#include <non_arduino_adaptations.h>


const char* ssid = "IxDIoT24";
const char* password = "70766086796235198563";

String serverName = "https://script.google.com/macros/s/AKfycbwnrhLZ43kTx7wClyokj3XlkfbS0S-qLC68UaDmlMi5AMMjSpQ_lUb37KZFVwFKQtmt/exec";  // Google Script von Incom hinzufügen
const String parameter = "?key=CampusMue12346&room=";

String responses[] = {};
char *ptr = NULL;

char* room;
int ble, rssi, iaq, voc, co2, nox;
float temp, hum, pm1, pm25, pm4, pm10;

void setup() {
  Serial.begin(115200);
  connectAP();

  Serial.println("\nConnected to WiFi");  
  serverName = serverName + parameter;
  Serial.println("\nConnected to WiFi");
}

void loop() {
  retreiveData("X001");
}

void retreiveData(char* _room) {
  // String retreiveURL = serverName;
  // strcpy(retreiveURL, room);
  String retreiveURL = serverName + _room;
  Serial.println("Trying to http get");
  if (WiFi.status() == WL_CONNECTED) {

    HTTPClient http;
    http.begin(retreiveURL);
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);

    int httpCode = http.GET();
    byte index;
    if (httpCode > 0) {
      String payload = http.getString();
      Serial.println("Received csv: " + payload);
      char* payload_cstr =  (char*)payload.c_str();
      // Serial.println("Casted to cstr: " + payload_cstr);

      CSV_Parser cp(payload_cstr, /*format*/ "sudududffududffff", false);

      // if(full) {
      //   for()
      // }

      room = (char*)cp[0];

      // Serial.println(String(room));
      Serial.printf("%s \n", room);

    } else {
      Serial.println("GET Failed, code: " + String(httpCode));
    }

    http.end();
    retreiveURL = "";
    //delay(15000);
    delay(0.1 * 60 * 1000); // one minunte interval
  } else {
    connectAP();
    retreiveData(room);
  }
}

void handleValues(String room, String date, int ble, int rssi, int iaq, float temp, float hum, int voc, int co2, int nox, float pm1, float pm25, float pm4, float pm10) {
  Serial.printf("Readings for %s at %s:\n", room, date);
  Serial.printf("ble %d\t rssi %d\t iaq %d\t temp %f°C\t rel.Hum. %f\%\t voc %d\t co2 %d\t nox %d\t pm1 %f\t pm2.5 %f\t pm4 %f\t pm10 %f\t", ble, rssi, iaq, temp, hum, voc, co2, nox, pm1, pm25, pm4, pm10);
  Serial.println();
  Serial.println();

  /*
   * DO SOMETHING WITH VALUES EACH TIME THEY GET UPDATED
   */
  
}

void connectAP() {
  Serial.print("WiFi not connected, trying to reconnect to given AP...");
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
}