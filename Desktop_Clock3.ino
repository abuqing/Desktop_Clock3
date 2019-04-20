
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

#define TRIGGER_PIN 16

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


volatile int state = HIGH;
volatile int count=0;
Ticker ticker;
volatile int state2 = HIGH;

int sig_pin = 3; // Touch sensor
int cdsVal = 0;

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_PIN_CS, TFT_PIN_DC, TFT_PIN_RST);

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
  Serial.println("Should save config");
  shouldSaveConfig = true;
}

void setup(void) {
  //  Serial.begin(9600);
  //  Serial.println("Desktop Clock service start");
  pinMode(sig_pin,INPUT);
  pinMode(TRIGGER_PIN, INPUT);
  
  // Initializer if using a 1.8" TFT screen:
  pinMode(TFT_BACKLIGHT, OUTPUT);
  digitalWrite(TFT_BACKLIGHT, state); // Backlight on
  tft.initR(INITR_BLACKTAB);      // Init ST7735S chip, black tab
  //  Serial.println(F("1.8 TFT screen Initialized"));
  tft.setRotation(1); //Rotate Display 0=0, 1=90, 2=180, 3=240
  tft.fillScreen(ST77XX_BLACK);
 
  // Initialize DS3231
  //  Serial.println("Initialize DS3231");
  clockDS.begin();
  
  ticker.attach(30, readTempTimer);

  if ( digitalRead(TRIGGER_PIN) == LOW ) {

  //Entering WiFi mode
    tft.setTextSize(1);
    tft.setCursor(0, 0);
    tft.println("WiFi mode ...");
    
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
    
    //sets timeout until configuration portal gets turned off
    //useful to make it all retry or go to sleep
    wifiManager.setTimeout(180);     //180 seconds
     
    if(!wifiManager.autoConnect("AutoConnectAP")){
    tft.setTextColor(CRIMSON);
    tft.println("Failed to connect and hit timeout.");
    tft.println("Please turn off the power and restart");
    delay(10000);
    //reset and try again, or maybe put it to deep sleep
    //    ESP.reset();
    ESP.deepSleep(0);
    delay(5000);
    }

  // WiFi connected
    tft.setTextColor(LIMEGREEN);
    tft.println("WiFi Connected");
    tft.setTextColor(ST77XX_WHITE);
    tft.println(WiFi.localIP());

  //read updated parameters
    strcpy(mqtt_server, custom_mqtt_server.getValue());
    strcpy(mqtt_port, custom_mqtt_port.getValue());
    strcpy(blynk_token, custom_blynk_token.getValue());
    strcpy(time_zone, custom_time_zone.getValue());

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

  // Initialize DS3231
  //  Serial.println("Initialize DS3231");
    clockDS.begin();

  // NTP Servr setting
    setupTime();

  // NTP Setting done
    tft.fillScreen(ST77XX_BLACK); tft.setCursor(0, 0);    
    tft.setTextColor(LIMEGREEN);
    tft.println("Success update settings");
    tft.setTextColor(ST77XX_WHITE);
    tft.print("Time Zone : +"); tft.println(time_zone);
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
    }
  }
}

void loop() {
  //  Serial.print("backlight state = ");
  //  Serial.println(state);
 
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

  
  if (state2){
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
    
    cdsVal = analogRead(A0); // input CDS sensor value

  //Automatic brightness adjustment
  if (state != 0) {
    if (cdsVal <= 5){
//      analogWrite(TFT_BACKLIGHT, 0);
      state = LOW;
     }else if (cdsVal <= 20){
      analogWrite(TFT_BACKLIGHT, 20);
     }else if (cdsVal <= 80){
      analogWrite(TFT_BACKLIGHT, 200);
     }else {
      analogWrite(TFT_BACKLIGHT, 255);
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
//        tft.setTextColor(ST77XX_BLUE);
        tft.setTextColor(DEEPSKYBLUE);
        tft.print("     Cool");
    } else if (clockDS.readTemperature() > 1){
        tft.setTextColor(LIGHTBLUE);
        tft.print("     Cold");
    } else {      
        tft.setTextColor(AZURE);
        tft.print("   Freezing");
    }

   if (WiFi.status() == WL_CONNECTED) {
    tft.setCursor(120, 30);
    tft.setTextColor(LIMEGREEN);
    tft.print("WiFi");
    }

//    Serial.print("Raw data: ");
//    Serial.print(dt.year);   Serial.print("-");
//    Serial.print(dt.month);  Serial.print("-");
//    Serial.print(dt.day);    Serial.print(" ");
//    Serial.print(dt.hour);   Serial.print(":");
//    Serial.print(dt.minute); Serial.print(":");
//    Serial.print(dt.second); Serial.println("");
//    Serial.print("Temperature: "); 
//    Serial.print(clock.readTemperature());
//    Serial.println(" *C ");
//    Serial.print("Lighting value : "); 
//    Serial.println(cdsVal);  
  
    state2 = !state2;
    }

  delay(50);
}

void readTempTimer()
{
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
    delay(450);
    tft.print(".");
    if(ntpCounter > 99) {
  //      Serial.println("ShutDown");
      tft.fillScreen(ST77XX_BLACK); tft.setCursor(0, 0);
      tft.setTextColor(CRIMSON);
      tft.println("Failed to connect NTP server.");
      tft.println("Please turn off the power and restart");
      delay(3000);
      ESP.deepSleep(0); delay(5000);
      }
    
    delay(50);
    t = time(NULL);
    tm = localtime(&t);
    ntpCounter++;
  }
  tft.println(""); tft.println("Sync RTC Time from NTP server");
  clockDS.setDateTime(tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);
   
//    dt = clockDS.getDateTime();
//    Serial.print("DS3231 : ");
//    Serial.print(dt.year);   Serial.print("-");
//    Serial.print(dt.month);  Serial.print("-");
//    Serial.print(dt.day);    Serial.print(" ");
//    Serial.print(dt.hour);   Serial.print(":");
//    Serial.print(dt.minute); Serial.print(":");
//    Serial.print(dt.second); Serial.println("");

}

