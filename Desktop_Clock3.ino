
/**************************************************************************
Wemos D1 mini

Touch sensor
GPIO 3 (RX) attachInterrupt

CDS sensor
ADC0 A0 Pin

RTC DS3231
GPIO 4 (D2)    SDA pin
GPIO 5 (D1)    SCL pin

Arduino-DS3231 library DS3231.h
https://github.com/jarzebski/Arduino-DS3231

TFT SPI 128x160 V1.1
Wemos D1 mini ---- 1.8 TFT SPI
GPIO 12 (D6)       RESET Pin
GPIO 2 (D4)        A0 Pin
GPIO 13 (D7)       SDA Pin
GPIO 14 (D5)       SCK Pin
GPIO 15 (D8)       CS Pin
GPIO 16 (D0)        LED Pin

Adafruit 1.8" SPI display library
https://github.com/adafruit/Adafruit-ST7735-Library/
Adafruit GFX graphics core library
https://github.com/adafruit/Adafruit-GFX-Library


Blynk library here:
 https://github.com/blynkkk/blynk-library/releases/latest

  Blynk is a platform with iOS and Android apps to control
  Arduino, Raspberry Pi and the likes over the Internet.
  You can easily build graphic interfaces for all your
  projects by simply dragging and dropping widgets.

    Downloads, docs, tutorials: http://www.blynk.cc
    Sketch generator:           http://examples.blynk.cc
    Blynk community:            http://community.blynk.cc
    Follow us:                  http://www.fb.com/blynkapp
                                http://twitter.com/blynk_app
V0 = Temperature value
V1 = Lighting value
V2 = LED Widget


 **************************************************************************/
#include <FS.h>                   //this needs to be first, or it all crashes and burns...
#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7735.h> // Hardware-specific library for ST7735
//#include <Adafruit_ST7789.h> // Hardware-specific library for ST7789
#include <SPI.h>
#include <Wire.h>
#include <DS3231.h>
#include <Ticker.h>
#include <time.h>

#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>         //https://github.com/tzapu/WiFiManager

#include <ArduinoJson.h>          //https://github.com/bblanchon/ArduinoJson

#include <BlynkSimpleEsp8266.h>
WidgetLED led1(V2);

#define TRIGGER_PIN 16

// Font file for display library
#include <Fonts/FreeSansBold24pt7b.h>
#include <Fonts/FreeSansBold18pt7b.h>
#include <Fonts/FreeSansBold12pt7b.h>
#include <Fonts/FreeSansBold9pt7b.h>

#define TFT_PIN_CS   15
#define TFT_PIN_DC   2
#define TFT_PIN_RST  12
#define TFT_BACKLIGHT  0 // Display backlight pin

//Font Color Settings
#define SLATEGRAY 0x634c
#define GAINSBORO 0xdedb
#define CRIMSON 0xd887
#define ORANGERED 0xfa20
#define LIMEGREEN 0x2e65
#define DEEPSKYBLUE 0x05ff
#define LIGHTBLUE 0xaebc
#define AZURE 0xefff

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_PIN_CS, TFT_PIN_DC, TFT_PIN_RST);

volatile int state = HIGH;
volatile int count=0;
Ticker ticker;
volatile boolean state2 = true;

int sig_pin = 3; // Touch sensor
int cdsVal = 0;

int UTC = 0;
DS3231 clockDS;
RTCDateTime dt;

//define your default values here, if there are different values in config.json, they are overwritten.
char mqtt_server[40] = "blynk-cloud.com";
char mqtt_port[6] = "8080";
char blynk_token[34] = "YOUR_BLYNK_TOKEN";
char time_zone[6] = "8";

//flag for saving data
bool shouldSaveConfig = false;

//callback notifying us of the need to save config
void saveConfigCallback () {
  tft.println("Should save config");
  shouldSaveConfig = true;
}

boolean state3 = false; //offline or wifi mode flag


void setup(void) {
  //  Serial.begin(9600);
  //  Serial.println("Desktop Clock service start");
  pinMode(sig_pin,INPUT);
  pinMode(TRIGGER_PIN, INPUT);
  
  // Initializer if using a 1.8" TFT screen:
  pinMode(TFT_BACKLIGHT, OUTPUT);
  digitalWrite(TFT_BACKLIGHT, state); // Backlight on
  tft.initR(INITR_BLACKTAB);      // Init ST7735S chip, black tab
  tft.setRotation(1); //Rotate Display 0=0, 1=90, 2=180, 3=240
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextSize(1);
  tft.setCursor(0, 0);
  tft.println(F("1.8 TFT screen Initialized"));
 
  // Initialize DS3231
  tft.println(F("RTC DS3231Initialized"));
  clockDS.begin();
  
  ticker.attach(30, readTempTimer);

  if ( digitalRead(TRIGGER_PIN) == LOW ) {

  //Entering WiFi mode
    tft.println("\nWiFi mode ...");
    state3 = true;
    
    if (SPIFFS.begin()) {
    tft.println("mounted file system");
    if (SPIFFS.exists("/config.json")) {
    //file exists, reading and loading
    tft.println("reading config file");
      File configFile = SPIFFS.open("/config.json", "r");
      if (configFile) {
    tft.println("opened config file");
        size_t size = configFile.size();
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);

        configFile.readBytes(buf.get(), size);
        DynamicJsonBuffer jsonBuffer;
        JsonObject& json = jsonBuffer.parseObject(buf.get());
        json.printTo(Serial);
        if (json.success()) {
    tft.println("parsed json");
          strcpy(mqtt_server, json["mqtt_server"]);
          strcpy(mqtt_port, json["mqtt_port"]);
          strcpy(blynk_token, json["blynk_token"]);
          strcpy(time_zone, json["time_zone"]);
        } else {
    tft.setTextColor(CRIMSON);
    tft.println("failed to load json config");
        }
        configFile.close();
      }
    }
  } else {
      tft.setTextColor(CRIMSON);
      tft.println("failed to mount FS");
  }
  //end read

  // The extra parameters to be configured (can be either global or just in the setup)
  // After connecting, parameter.getValue() will get you the configured value
  // id/name placeholder/prompt default length
  WiFiManagerParameter custom_mqtt_server("server", "mqtt server", mqtt_server, 40);
  WiFiManagerParameter custom_mqtt_port("port", "mqtt port", mqtt_port, 6);
  WiFiManagerParameter custom_blynk_token("blynk", "blynk token", blynk_token, 32);
  WiFiManagerParameter custom_time_zone("UTC", "Time Zone", time_zone, 6);

  //WiFiManager
  //Local intialization. Once its business is done, there is no need to keep it around
    WiFiManager wifiManager;

  //set config save notify callback
    wifiManager.setSaveConfigCallback(saveConfigCallback);

  //add all your parameters here
    wifiManager.addParameter(&custom_mqtt_server);
    wifiManager.addParameter(&custom_mqtt_port);
    wifiManager.addParameter(&custom_blynk_token);
    wifiManager.addParameter(&custom_time_zone);

    //fetches ssid and pass from eeprom and tries to connect
    //if it does not connect it starts an access point with the specified name
    //and goes into a blocking loop awaiting configuration
    wifiManager.autoConnect("myDesktopClock");

  // WiFi connected
    tft.setTextColor(LIMEGREEN);
    tft.println("\nWiFi Connected");
    tft.setTextColor(ST77XX_WHITE);
    tft.println(WiFi.localIP());

  //read updated parameters
    strcpy(mqtt_server, custom_mqtt_server.getValue());
    strcpy(mqtt_port, custom_mqtt_port.getValue());
    strcpy(blynk_token, custom_blynk_token.getValue());
    strcpy(time_zone, custom_time_zone.getValue());

  //still not connected to the Blynk server yet
  //it will try to connect when it reaches first Blynk.run() or Blynk.connect() call
    Blynk.config(blynk_token, mqtt_server, atoi(mqtt_port));

  //save the custom parameters to FS
  if (shouldSaveConfig) {
    tft.println("saving config");
    DynamicJsonBuffer jsonBuffer;
    JsonObject& json = jsonBuffer.createObject();
    json["mqtt_server"] = mqtt_server;
    json["mqtt_port"] = mqtt_port;
    json["blynk_token"] = blynk_token;
    json["time_zone"] = time_zone;

    File configFile = SPIFFS.open("/config.json", "w");
    if (!configFile) {
      tft.println("failed to open config file for writing");
    }

//    json.printTo(Serial);
    json.printTo(configFile);
    configFile.close();
    //end save
  }
    
    UTC = atoi(time_zone);
    configTime( 3600* UTC, 0, "ntp.nict.jp", "ntp.jst.mfeed.ad.jp");


  // NTP Servr setting
    setupTime();

  // NTP Setting done
    tft.fillScreen(ST77XX_BLACK); tft.setCursor(0, 0);    
    tft.setTextColor(LIMEGREEN);
    tft.println("Success update settings");
    tft.setTextColor(ST77XX_WHITE);
    tft.print("\nTime Zone : +"); tft.println(time_zone);
    tft.print("mqtt_server : "); tft.println(mqtt_server);
    tft.print("mqtt_port : "); tft.println(mqtt_port);
    tft.println("blynk_token :"); tft.println(blynk_token);
    
    delay(5000);
  //Reset WiFi settings with touch switch
    if (digitalRead(sig_pin) == HIGH) {
    wifiManager.resetSettings();
    tft.setTextColor(ORANGERED);
    tft.print("\nReset WiFi settings");
    delay(5000);
    state3 = false;
    }
  }
}

void loop() {
  if (state3 == true) {
    Blynk.run();
  }
  
  if (digitalRead(sig_pin) == HIGH) {
  if (count >= 5){
    state = !state;
    digitalWrite(TFT_BACKLIGHT, state);
    count = 0;
    delay(1000);
  }
    count ++;
  }

  // Reset count after 500 milliseconds
   static unsigned long last_time =0;
   if (count == 1){
    last_time = millis();
    count ++;
   }
   if (millis() - last_time > 500) {
    count = 0;
    last_time =0;
   }


  if (state2 == true){
    tft.fillScreen(ST77XX_BLACK);
    tft.setTextColor(GAINSBORO);
    
    tft.drawRoundRect(15,35,135,40,10,GAINSBORO);
    tft.setTextSize(0);
    
    dt = clockDS.getDateTime();
    tft.setFont(&FreeSansBold12pt7b);
    tft.setCursor(2, 22);
    tft.print(dt.year);
    tft.print('/');
    tft.print(dt.month);
    tft.print('/');
    tft.print(dt.day);
  
    tft.setFont(&FreeSansBold24pt7b);
    tft.setCursor(10,70);
    tft.print(" ");
    // Add blank for single digit
    if (dt.hour < 10){tft.print(" ");}
    tft.print(dt.hour);
    tft.print(':');
    // Add 0 for single digit
    if (dt.minute < 10){tft.print("0");}
    tft.print(dt.minute);
  
    tft.setFont(&FreeSansBold9pt7b);
    tft.setCursor(20, 110);
    tft.print("Temp : ");
    tft.print(clockDS.readTemperature());
    tft.println(" C");
    


  //Automatic brightness adjustment
    cdsVal = analogRead(A0); // input CDS sensor value
    
    if (state != 0) {
      if (cdsVal <= 8){
        state = 0;
        digitalWrite(TFT_BACKLIGHT, state);
        //analogWrite(TFT_BACKLIGHT, 0);
       }else if (cdsVal <= 20){
        analogWrite(TFT_BACKLIGHT, 20);
       }else if (cdsVal <= 60){
        analogWrite(TFT_BACKLIGHT, 100);
       }else {
        digitalWrite(TFT_BACKLIGHT, 1);
      }
    }
   
    tft.setCursor(20, 125);
    tft.print("Lighting : ");
    tft.print(cdsVal);
  
    tft.setCursor(20, 92);
    if (clockDS.readTemperature() > 35){
       tft.setTextColor(CRIMSON);
       tft.print("   Hot !!!");
    } else if (clockDS.readTemperature() > 29){
       tft.setTextColor(ORANGERED);
       tft.print("    Warm");
    } else if (clockDS.readTemperature() > 21){
        tft.setTextColor(LIMEGREEN);
        tft.print(" Comfortable");
    } else if (clockDS.readTemperature() > 15){
        tft.setTextColor(DEEPSKYBLUE);
        tft.print("     Cool");
    } else if (clockDS.readTemperature() > 1){
        tft.setTextColor(LIGHTBLUE);
        tft.print("     Cold");
    } else {      
        tft.setTextColor(AZURE);
        tft.print("   Freezing");
    }

  if (state3 == true) {
    if (WiFi.status() == WL_CONNECTED) {
      tft.setCursor(120, 30);
      tft.setTextColor(LIMEGREEN);
      tft.print("WiFi");
      delay(100);
      }
    if (Blynk.connected()){
      SendData_blynk();
      tft.setCursor(120, 15);
      tft.setTextColor(ORANGERED);
      tft.print("BLK");
      }
  }
  
    state2 = !state2;
  }

  delay(50);
}

void readTempTimer() {
 state2 = !state2;
}

void setupTime() {
  time_t t;
  struct tm *tm;
  t = time(NULL);
  tm = localtime(&t);
  tft.setTextColor(ST77XX_WHITE);
  tft.print("Connecting NTP Server.");
  
  int ntpCounter = 0;
    while (t <= 1000000) {
      delay(500);
      tft.print(".");
      if(ntpCounter > 60) {
        tft.fillScreen(ST77XX_BLACK); tft.setCursor(0, 0);
        tft.setTextColor(CRIMSON);
        tft.println("\nFailed to connect NTP server.");
        delay(5000);
        //ESP.deepSleep(0); delay(5000);
        break;
       }
      delay(50);
      t = time(NULL);
      tm = localtime(&t);
      ntpCounter++;
    }
    if (ntpCounter < 60){
      tft.setTextColor(LIMEGREEN);
      tft.println(""); tft.println("\nSync RTC Time from NTP server");
      clockDS.setDateTime(tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);
    }
}

void SendData_blynk() {
  // V0 = Temperature value
    Blynk.virtualWrite(V0, clockDS.readTemperature());
  // V1 = Lighting value
    Blynk.virtualWrite(V1, cdsVal);
  // TFT Display backlight state
  if (state == 0) {
    led1.off();
  } else {
    led1.on();
  }
}

