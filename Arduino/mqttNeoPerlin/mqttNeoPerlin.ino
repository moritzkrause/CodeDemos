#include <LittleFS.h>

#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>
#include <PubSubClient.h>

#include <FastLED.h>




/*
 * WEBSERVER
 */
// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

// Set mDNS-Adress -> [dns_name].local
const char* dns_name = "Make A10";
const char* dns_name_setup = "setup";

// Search for parameter in HTTP POST request
const char* PARAM_INPUT_1 = "ssid";
const char* PARAM_INPUT_2 = "pass";
const char* PARAM_INPUT_3 = "name";

//Variables to save values from HTML form
String ssid;
String pass;
String gridname = "Make A10";
String ip = "192.168.1.200";
String gateway = "192.168.1.1";

// File paths to save input values permanently
const char* ssidPath = "/ssid.txt";
const char* passPath = "/pass.txt";
const char* namePath = "/name.txt";

IPAddress localIP;
IPAddress localGateway;
IPAddress subnet(255, 255, 0, 0);

unsigned long previousMillis = 0;
const long interval = 10000;
boolean restart = false;
bool wificonnected = false;


/*
 * MQTT
 */
const char* mqtt_server = "homebridgedemo.cloud.shiftr.io";
WiFiClient wificlient;
PubSubClient client(wificlient);


/*
 * VALUES
 */
//flag for color shift
int IAQ_Range = 4; // 0 bad, 1 okay, 2 good, 4 init
int IAQ_Prev = 4;

//flag and val for dimming brightness on change
int count = -100;
bool iaq_change = false;

// container for values
String show = "";




/*
 * MATRIX UNITS
 */
uint8_t Width  = 16;
uint8_t Height = 16;
uint8_t CentreX =  (Width / 2) - 1;
uint8_t CentreY = (Height / 2) - 1;
#define NUM_LEDS      256
uint8_t BRIGHTNESS = 100;
CRGB leds[NUM_LEDS];
/*
 * NOISE UNITS
 */
// max movement speed
float speeed  = 3;
// total field scale
int sclX    =  12;
int sclY    =  12;
// "speed" of animation
int medianX =  40;
int medianY =  40;

// nicer speed than median,  fast     to         slow
float intensity = 3.0;     // min 1.5, default 3, max 6

// blue factor for temperature to color correction
float bf = 1.0;

int c_low = 0;

int ts_start =   0;   //   0
int ts_low   =  64;   //  64
int ts_mid   = 128;   // 128
int ts_high  = 192;   // 192
int ts_end   = 255;   // 255

DEFINE_GRADIENT_PALETTE( pit ) {
  ts_start, c_low, c_low, c_low,
  ts_low,     255,   255,   255,  // b r g
  ts_mid,   c_low, c_low, c_low,
  ts_high,    255,   255,   255,  // b r g
  ts_end,   c_low, c_low, c_low
};





/*
 * SETUP
 */
 
void setup() {
  Serial.begin(115200);
  while(!Serial) { };

  pinMode(LED_BUILTIN, OUTPUT);

  initFS();

  /*
   * LED INIT
   */
  FastLED.addLeds<WS2811,2,BGR>(leds,NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS); 
  FastLED.setMaxRefreshRate(400);


  /*
   * WIFIMANAGER
   */
  // Load values saved in LittleFS
  ssid = readFile(LittleFS, ssidPath);
  pass = readFile(LittleFS, passPath);
  gridname = readFile(LittleFS, namePath);
  Serial.println(ssid);
  Serial.println(pass);

  FastLED.clear();
  FastLED.show();

  /*
   * SUCCESS
   */
  if (initWiFi()) {
    digitalWrite(LED_BUILTIN, LOW);
    client.setServer(mqtt_server, 1883);
    Serial.println("connecting mqtt");
    Serial.print(mqtt_server);
    Serial.print("    ");
    Serial.print(gridname.c_str());
    Serial.print("    ");
    Serial.print("homebridgedemo");
    Serial.print("  —  ");
    Serial.print("aucJ0EuqEmP1PhRk");
    Serial.print("    ");
    while (!client.connect(gridname.c_str(), "homebridgedemo", "aucJ0EuqEmP1PhRk")) {
      Serial.print(".");
      delay(1000);
    }
    client.setCallback(callback);
    client.subscribe("air/hum");
    client.subscribe("air/iaq");
    client.subscribe("air/temp");
    //client.subscribe("air/details/co2");
    //client.subscribe("air/details/voc");
  }
  /*
   * FAIL
   */
  else {
    Serial.println("Setting up Access Point");
    Serial.println(dns_name);
    WiFi.softAP(dns_name, NULL);

    IPAddress IP = WiFi.softAPIP();
    Serial.print("Access Point IP address: ");
    Serial.println(IP);

    if (MDNS.begin(dns_name_setup)) {
      Serial.println("DNS gestartet, erreichbar unter: ");
      Serial.println("http://" + String(dns_name_setup) + ".local/");
    }

    // Web Server Root URL
    server.on("/", HTTP_GET, [](AsyncWebServerRequest * request) {
      Serial.println("http GET");
      request->send(LittleFS, "/wifimanager.html", "text/html");
    });

    // Route to load style.css file
    server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest * request) {
      request->send(LittleFS, "/style.css", "text/css");
    });

    server.serveStatic("/", LittleFS, "/");

    Serial.println("Server running");

    server.on("/", HTTP_POST, [](AsyncWebServerRequest * request) {
      int params = request->params();
      for (int i = 0; i < params; i++) {
        AsyncWebParameter* p = request->getParam(i);
        if (p->isPost()) {
          // HTTP POST ssid value
          if (p->name() == PARAM_INPUT_1) {
            ssid = p->value().c_str();
            Serial.print("SSID set to: ");
            Serial.println(ssid);
            writeFile(LittleFS, ssidPath, ssid.c_str());
          }
          // HTTP POST pass value
          if (p->name() == PARAM_INPUT_2) {
            pass = p->value().c_str();
            Serial.print("Password set to: ");
            Serial.println(pass);
            writeFile(LittleFS, passPath, pass.c_str());
          }
          // HTTP POST name value
          if (p->name() == PARAM_INPUT_3) {
            //gridname = p->value().c_str() + WiFi.macAddress();
            gridname = p->value().c_str();
            Serial.print("gridname set to: ");
            Serial.println(gridname);
            writeFile(LittleFS, namePath, gridname.c_str());
          }
        }
      }
      
      delay(300);

      restart = true;

      request->send(LittleFS, "/success.html", String(), false, processor);
    });

    server.begin();
  }
  
}


/*
 * INIT WIFI HELPER
 */
bool initWiFi() {
  if (ssid == "") {
    Serial.println("Undefined SSID.");
    return false;
  }

  digitalWrite(LED_BUILTIN, HIGH);

  WiFi.mode(WIFI_STA);
  localIP.fromString("192.168.1.200");
  localGateway.fromString("192.168.1.1");
  WiFi.begin(ssid.c_str(), pass.c_str());

  Serial.println("Connecting to WiFi...");
  // TODO: DO_WHILE? *********
  delay(20000);
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Failed to connect.");
    return false;
  }

  Serial.println(WiFi.localIP());
  Serial.println(WiFi.macAddress());
  wificonnected;
  return true;
}


/*
 * NOISE PARAM AND BUFFER
 */
#define NUM_LAYERS 1
uint32_t x[NUM_LAYERS];
uint32_t y[NUM_LAYERS];
uint32_t z[NUM_LAYERS];
uint32_t scale_x[NUM_LAYERS];
uint32_t scale_y[NUM_LAYERS];
uint8_t  noise[1][16][16];

/*
 * CALLBACK: UPDATE NOISE VARIABLES HERE
 */
void callback(char* topic, byte* payload, unsigned int length) {

  Serial.print("Message arrived in topic: ");
  Serial.println(topic);
  String messageTemp;

  Serial.print("Message:");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
    messageTemp += (char)payload[i];
  } 

  show = messageTemp;

  if ((String)topic == "air/iaq") {
    if(show == "calibrating") {
      IAQ_Range = 4;
      BRIGHTNESS = 30;
    }
    else {
      if (show.toInt() == 0) {
        IAQ_Range = 4;
        BRIGHTNESS = 30;
      }
      else if (show.toInt() <= 50) {
        IAQ_Range = 2;
        BRIGHTNESS = 100;
      }
      else if (show.toInt() > 50 && show.toInt() <= 150) {
        IAQ_Range = 1;
        BRIGHTNESS = 100;
      }
      else if (show.toInt() > 150) {
        IAQ_Range = 0;
        BRIGHTNESS = 100;
      }
    }
    
    if(IAQ_Prev != IAQ_Range) {
      iaq_change = true;
    }
    IAQ_Prev = IAQ_Range;
  }

  if ((String)topic == "air/hum") {
    //speeed = map(show.toInt(), 15, 30, 1.5, 2.5);
    intensity = map(show.toInt(), 0, 100, 2.0, 12.0);
    BRIGHTNESS = 100;
  }
  if ((String)topic == "air/temp") {
    //medianX = map(show.toInt(), 0, 100, 120, 300);
    bf = map(show.toInt()-10.5, 10, 32, 1.4, 0.6);
    BRIGHTNESS = 100;
  }
  
  Serial.println();
  Serial.println(show.toInt());
  Serial.println("-----------------------");
}

/*
 * LOOP
 */
void loop() {
  long wl_duration;
  
  if (wificonnected && !client.connected()) {
    client.connect(gridname.c_str(), "homegrid", "GPmFHSj5VKDTPi3B");
    //delay(4);

    client.subscribe("air/hum");
    client.subscribe("air/iaq");
    client.subscribe("air/temp");
    //client.subscribe("air/details/co2");
    //client.subscribe("air/details/voc");
  }
  else if (client.connected()) {
    if (millis() <= previousMillis + interval) {
      client.publish("keepalive", "true");
      previousMillis = millis();
      Serial.print("Interval, Loop — ");
      Serial.println(show);
    }
  }

  if (restart) {
    delay(5000);
    ESP.restart();
  }

  noise_noise1();
  
  if(iaq_change && count <= BRIGHTNESS) {
    count++;
    if(count < 0) {
      FastLED.setBrightness(count * -1);
    }
    else if (count >= 0) {
      FastLED.setBrightness(count);
    }
  } else {
    count = BRIGHTNESS * -1;
    iaq_change = false;
    FastLED.setBrightness(BRIGHTNESS);
  }
  client.loop();

  
}



/*
 * NOISE HELPERS
 */
// indexing within a zigzag matrix
uint16_t XY( uint8_t x, uint8_t y) {
  uint16_t i;
  if ( y & 0x01) {
    uint8_t reverseX = (Width - 1) - x;
    i = (y * Width) + reverseX;
  } else {
    i = (y * Width) + x;
  }
  return i;
}


/*
 * COLOR CORRECTION
 */
void adjust_gamma()
{
  for (uint16_t i = 0; i < NUM_LEDS; i++)
  {
    if(IAQ_Range == 4) { // init
      leds[i].r = dim8_video(leds[i].r)/(10*bf);   // blue
      leds[i].g = dim8_video(leds[i].g)/10;   // red
      leds[i].b = dim8_video(leds[i].b)/10;   // green
    }
    else if(IAQ_Range == 0) { // v bad
      leds[i].r = dim8_video(leds[i].r)/(60*bf);   // blue
      leds[i].g = dim8_video(leds[i].g)/3;    // red
      leds[i].b = dim8_video(leds[i].b)/45;   // green
    }
    else if(IAQ_Range == 1) { // okay
      leds[i].r = dim8_video(leds[i].r)/(30*bf);   // blue
      leds[i].g = dim8_video(leds[i].g)/12;   // red
      leds[i].b = dim8_video(leds[i].b)/14;   // green
    }
    else if(IAQ_Range == 2) { // noice
      leds[i].r = dim8_video(leds[i].r)/(32*bf);   // blue
      leds[i].g = dim8_video(leds[i].g)/46;   // red
      leds[i].b = dim8_video(leds[i].b)/6;    // green
    }
  }
}

/*
 * NOISE CALC
 */
void noise_noise1() {

  CRGBPalette16 Pal( pit );

  x[0] = x[0] + (speeed * noise[0][0][0]) - medianX;
  y[0] = y[0] + (speeed * noise[0][Width-1][0]) - medianY;
  z[0] += 1 + ((noise[0][0][Height-1]) / intensity);
  scale_x[0] = 1000 * sclX + (noise[0][0][CentreY] * 16);
  scale_y[0] = 1000 * sclY + (noise[0][Width-1][CentreY] * 16);

  //calculate the noise data
  uint8_t layer = 0;
  for (uint8_t i = 0; i < Width; i++) {
    uint32_t ioffset = scale_x[layer] * (i - CentreX);
    for (uint8_t j = 0; j < Height; j++) {
      uint32_t joffset = scale_y[layer] * (j - CentreY);
      uint16_t data = inoise16(x[layer] + ioffset, y[layer] + joffset, z[layer]);
      // limit the 16 bit results to the interesting range
      if (data < 11000) data = 11000;
      if (data > 51000) data = 51000;
      // normalize
      data = data - 11000;
      // scale down that the result fits into a byte
      data = data / 161;
      // store the result in the array
      noise[layer][i][j] = data;
    }
  }

  //map the colors
  for (uint8_t y = 0; y < Height; y++) {
    for (uint8_t x = 0; x < Width; x++) {
      CRGB overlay = CHSV(noise[0][y][x], 255, noise[0][x][y]);
      leds[XY(x, y)] = ColorFromPalette( Pal, noise[0][Width-1][Height-1] + noise[0][x][y]) + overlay;
    }
  }

  //color correction
  adjust_gamma();

  // print
  FastLED.show();
}





/*
 * LITTLE FS PROCESSOR
 */
String processor(const String& var) {
  Serial.println(var);
  if (var == "gridname") {
    return String(gridname);
  }
  return String();
}

/*
 * INIT LITTLEFS
 */
// Initialize LittleFS
void initFS() {
  if (!LittleFS.begin()) {
    Serial.println("An error has occurred while mounting LittleFS");
  }
  else {
    Serial.println("LittleFS mounted successfully");
  }
}
// Read File from LittleFS
String readFile(fs::FS &fs, const char * path) {
  Serial.printf("Reading file: %s\r\n", path);
  File file = fs.open(path, "r");
  if (!file || file.isDirectory()) {
    Serial.println("- failed to open file for reading");
    return String();
  }
  String fileContent;
  while (file.available()) {
    fileContent = file.readStringUntil('\n');
    break;
  }
  file.close();
  return fileContent;
}
// Write file to LittleFS
void writeFile(fs::FS &fs, const char * path, const char * message) {
  Serial.printf("Writing file: %s\r\n", path);
  File file = fs.open(path, "w");
  if (!file) {
    Serial.println("- failed to open file for writing");
    return;
  }
  if (file.print(message)) {
    Serial.println("- file written");
  } else {
    Serial.println("- frite failed");
  }
  file.close();
}

void blinkLED(int count) {
  for (int i = 0; i <= count; i++) {
      digitalWrite(LED_BUILTIN, HIGH);
      delay(100);
      digitalWrite(LED_BUILTIN, LOW);
      delay(100);
    }
}
