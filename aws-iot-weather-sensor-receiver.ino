#include "FS.h"
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

#include <Wire.h>
#include <SPI.h>
#include <Adafruit_BMP280.h>
#include <Adafruit_Sensor.h>
#include "Adafruit_SHT31.h"

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define NUMFLAKES     10 // Number of snowflakes in the animation example

#define LOGO_HEIGHT   16
#define LOGO_WIDTH    16
static const unsigned char PROGMEM logo_bmp[] =
{ B00000000, B11000000,
  B00000001, B11000000,
  B00000001, B11000000,
  B00000011, B11100000,
  B11110011, B11100000,
  B11111110, B11111000,
  B01111110, B11111111,
  B00110011, B10011111,
  B00011111, B11111100,
  B00001101, B01110000,
  B00011011, B10100000,
  B00111111, B11100000,
  B00111111, B11110000,
  B01111100, B11110000,
  B01110000, B01110000,
  B00000000, B00110000 };


#include <ArduinoJson.h>

// Update these with values suitable for your network.

const String ssid = "grwb";
const String password = "lemonslimes";

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");

#define BMP_SCK  (14)
#define BMP_MISO (13)
#define BMP_MOSI (12)
#define BMP_CS   (3)

#define MQTT_KEEP_ALIVE (900)
#define MQTT_TIMEOUT    (900)
 
Adafruit_BMP280 bmp(BMP_CS, BMP_MOSI, BMP_MISO,  BMP_SCK);
Adafruit_SHT31 sht31 = Adafruit_SHT31();


const char* AWS_endpoint = "a17xxrmnoajneh-ats.iot.us-west-2.amazonaws.com"; //MQTT broker ip
const String AWSThingId = "e72e152858"; // unique thing identifier for AWS IoT

class Reading {

  float temperature;
  float humidity;
  float pressure;

  public:
    
    void init () {
    }
    
    void setTemperature (float value) {
      temperature = value;
    }
    
    void setHumidity (float value) {
      humidity = value;
    }
    
    void setPressure (float value) {
      pressure = value;
    }

    float getTemperature() {
      return temperature;
    }

    float getHumidity() {
      return humidity;
    }

    String getJson() {
  
      StaticJsonDocument<500> doc;
    
      doc["temperature"] = temperature;
      doc["humidity"] = humidity;
      doc["pressure"] = pressure;
    
      String json;
      serializeJson(doc, json);

      return json;

    }

    

};

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
  StaticJsonDocument<200> doc;

  // Deserialize the JSON document
  DeserializationError error = deserializeJson(doc, payload);

  // Test if parsing succeeds.
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.c_str());
    return;
  }

  // Fetch values.
  //
  // Most of the time, you can rely on the implicit casts.
  // In other case, you can do doc["time"].as<long>();
  int swtch = doc["switch"];
  boolean state = doc["state"];
//  long time = doc["time"];
//  double latitude = doc["data"][0];
//  double longitude = doc["data"][1];

  Serial.print("Switch: ");
  Serial.print(swtch);
  Serial.print(" | state: ");
  Serial.print(state);
  Serial.print("\n");

}

WiFiClientSecure espClient;
PubSubClient client(AWS_endpoint, 8883, callback, espClient); //set  MQTT port number to 8883 as per //standard
long lastMsg = 0;
char msg[50];
int value = 0;

void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  espClient.setBufferSizes(512, 512);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid.c_str(), password.c_str());

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  timeClient.begin();
  while(!timeClient.update()){
    timeClient.forceUpdate();
  }

  espClient.setX509Time(timeClient.getEpochTime());

}


void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    client.setKeepAlive(MQTT_KEEP_ALIVE);
    client.setSocketTimeout(MQTT_TIMEOUT);
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (clietim0nt.connect(AWSThingId.c_str())) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish("outTopic", "hello world");
      // ... and resubscribe
      client.subscribe("/environment/sensor/weather");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");

      char buf[256];
      espClient.getLastSSLError(buf,256);
      Serial.print("WiFiClientSecure SSL error: ");
      Serial.println(buf);

      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void publishSensorData(Reading reading) {

  String json = reading.getJson();  
  client.publish("/environment/sensor/weather", json.c_str());
//  client.publish("/environment/sensor/weather", reading.getJson());

  Serial.println(json);

}

void setup() {

  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3C for 128x32
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

  display.clearDisplay();


  // Show initial display buffer contents on the screen --
  // the library initializes this with an Adafruit splash screen.
  display.display();
  delay(2000); // Pause for 2 seconds

//  // Clear the buffer
  display.clearDisplay();
//
//  // Draw a single pixel in white
//  display.drawPixel(10, 10, SSD1306_WHITE);
//
//  // Show the display buffer on the screen. You MUST call display() after
//  // drawing commands to make them visible on screen!
//  display.display();
//  delay(2000);
//  // display.display() is NOT necessary after every single drawing command,
//  // unless that's what you want...rather, you can batch up a bunch of
//  // drawing operations and then update the screen all at once by calling
//  // display.display(). These examples demonstrate both approaches...
//
////  testdrawline();      // Draw many lines
////
////  testdrawrect();      // Draw rectangles (outlines)
////
////  testfillrect();      // Draw rectangles (filled)
////
////  testdrawcircle();    // Draw circles (outlines)
////
////  testfillcircle();    // Draw circles (filled)
////
////  testdrawroundrect(); // Draw rounded rectangles (outlines)
////
////  testfillroundrect(); // Draw rounded rectangles (filled)
////
////  testdrawtriangle();  // Draw triangles (outlines)
////
////  testfilltriangle();  // Draw triangles (filled)
//
//  testdrawchar();      // Draw characters of the default font
//
//  testdrawstyles();    // Draw 'stylized' characters
//
////  testscrolltext();    // Draw scrolling text
//
////  testdrawbitmap();    // Draw a small bitmap image
//
//  // Invert and restore display, pausing in-between
//  display.invertDisplay(true);
//  delay(1000);
//  display.invertDisplay(false);
//  delay(1000);
//
////  testanimate(logo_bmp, LOGO_WIDTH, LOGO_HEIGHT); // Animate bitmaps
  

  String certificate = "/" + AWSThingId + ".cert.der";
  String privateKey = "/" + AWSThingId + ".private.der";
  String caCertificate = "/ca.der";
  
  Serial.begin(115200);
  Serial.setDebugOutput(true);

//   if (!bmp.begin()) {
//    Serial.println(F("Could not find a valid BMP280 sensor, check wiring!"));
//    while (1);
//  }
//
//  /* Default settings from datasheet. */
//  bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,     /* Operating Mode. */
//                  Adafruit_BMP280::SAMPLING_X2,     /* Temp. oversampling */
//                  Adafruit_BMP280::SAMPLING_X16,    /* Pressure oversampling */
//                  Adafruit_BMP280::FILTER_X16,      /* Filtering. */
//                  Adafruit_BMP280::STANDBY_MS_500); /* Standby time. */  
//
//  if (! sht31.begin(0x44)) {   // Set to 0x45 for alternate i2c addr
//    Serial.println("Couldn't find SHT31");
//    while (1) delay(1);
//  }

  setup_wifi();
  delay(1000);
  if (!SPIFFS.begin()) {
    Serial.println("Failed to mount file system");
    return;
  }

  // Load certificate file
  File cert = SPIFFS.open(certificate, "r"); //replace cert.crt eith your uploaded file name
  if (!cert) {
    Serial.println("Failed to open cert file");
  }
  else
    Serial.println("Success to open cert file");

  delay(1000);

  if (espClient.loadCertificate(cert))
    Serial.println("cert loaded");
  else
    Serial.println("cert not loaded");

  // Load private key file
  File private_key = SPIFFS.open(privateKey, "r"); //replace private eith your uploaded file name
  if (!private_key) {
    Serial.println("Failed to open private cert file");
  }
  else
    Serial.println("Success to open private cert file");

  delay(1000);

  if (espClient.loadPrivateKey(private_key))
    Serial.println("private key loaded");
  else
    Serial.println("private key not loaded");



    // Load CA file
    File ca = SPIFFS.open(caCertificate, "r"); //replace ca eith your uploaded file name
    if (!ca) {
      Serial.println("Failed to open ca ");
    }
    else
    Serial.println("Success to open ca");

    delay(1000);

    if(espClient.loadCACert(ca))
    Serial.println("ca loaded");
    else
    Serial.println("ca failed");

}

void loop() {

//  if (!client.connected()) {
//    reconnect();
//  }

  Reading reading = Reading();

  reading.setTemperature(27.0);
  reading.setHumidity(67.0);
  reading.setPressure(99999.1);

//  publishSensorData(reading);
//  drawTemperature(reading);

  client.loop();
  
//  delay(1000);

}

void drawTemperature(Reading reading) {

  Serial.println("display");

  float c = reading.getTemperature();
  float f = c * 9.0 / 5.0 + 32;
  float h = reading.getHumidity();

  display.clearDisplay();

  display.setTextSize(1);             // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE);        // Draw white text
  display.setCursor(0,0);             // Start at top-left corner

  display.print(c); display.println("*C");
  display.print(f); display.println("*F");
  display.print(h); display.println("%");
  display.display();
  
}
