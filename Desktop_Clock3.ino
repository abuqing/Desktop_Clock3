
/**************************************************************************
Wemos D1 mini

Touch sensor
GPIO 3 (RX) attachInterrupt

CDS sensor
ADC0 A0 Pin

RTC DS3231
GPIO 4 (D2)    SDA pin
GPIO 5 (D1)    SCL pin

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

#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7735.h> // Hardware-specific library for ST7735
//#include <Adafruit_ST7789.h> // Hardware-specific library for ST7789
#include <SPI.h>
#include <Wire.h>
#include <DS3231.h>
#include <Ticker.h>

#include <Fonts/FreeSansBold24pt7b.h>
#include <Fonts/FreeSansBold18pt7b.h>
#include <Fonts/FreeSansBold12pt7b.h>
#include <Fonts/FreeSansBold9pt7b.h>

#define TFT_PIN_CS   15
#define TFT_PIN_DC   2
#define TFT_PIN_RST  12
#define TFT_BACKLIGHT  16 // Display backlight pin

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

DS3231 clock;
RTCDateTime dt;

void setup(void) {
  //  Serial.begin(9600);
  //  Serial.println("Desktop Clock service start");
  pinMode(sig_pin,INPUT);
  
  // Initializer if using a 1.8" TFT screen:
  pinMode(TFT_BACKLIGHT, OUTPUT);
  digitalWrite(TFT_BACKLIGHT, state); // Backlight on
  tft.initR(INITR_BLACKTAB);      // Init ST7735S chip, black tab
  //  Serial.println(F("1.8 TFT screen Initialized"));
  tft.setRotation(1); //Rotate Display 0=0, 1=90, 2=180, 3=240
  tft.fillScreen(ST77XX_BLACK);
 
  // Initialize DS3231
  //  Serial.println("Initialize DS3231");
  clock.begin();
  // Set sketch compiling time
//clock.setDateTime(__DATE__, __TIME__);
  ticker.attach(30, readTempTimer);

  //  Serial.println("Initialize Touch Sensor");
  //  attachInterrupt(sig_pin,displayOnOff,RISING);
    
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

  //Automatic brightness adjustment
  if (state != 0) {
    if (cdsVal <= 10){
      analogWrite(TFT_BACKLIGHT, 0);
     }else if (cdsVal <= 20){
      analogWrite(TFT_BACKLIGHT, 20);
     }else if (cdsVal <= 50){
      analogWrite(TFT_BACKLIGHT, 100);
     }else if (cdsVal <= 80){
      analogWrite(TFT_BACKLIGHT, 180);
     }else {
      analogWrite(TFT_BACKLIGHT, 255);
    }
   }

  
  if (state2){
    tft.fillScreen(ST77XX_BLACK);
    tft.setTextColor(GAINSBORO);
    
    tft.drawRoundRect(15,35,135,40,10,GAINSBORO);
    tft.setTextSize(0);
    
    dt = clock.getDateTime();
    tft.setFont(&FreeSansBold12pt7b);
    tft.setCursor(5, 25);
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
    tft.print(clock.readTemperature());
    tft.println(" C");
    
    cdsVal = analogRead(A0); // input CDS sensor value

    tft.setCursor(20, 125);
    tft.print("Lighting : ");
    tft.print(cdsVal);
  
    tft.setCursor(20, 95);
    if (clock.readTemperature() > 35){
       tft.setTextColor(CRIMSON);
       tft.print("   Hot !!!");
    } else if (clock.readTemperature() > 29){
       tft.setTextColor(ORANGERED);
       tft.print("    Warm");
    } else if (clock.readTemperature() > 21){
        tft.setTextColor(LIMEGREEN);
        tft.print(" Comfortable");
    } else if (clock.readTemperature() > 15){
//        tft.setTextColor(ST77XX_BLUE);
        tft.setTextColor(DEEPSKYBLUE);
        tft.print("     Cool");
    } else if (clock.readTemperature() > 1){
        tft.setTextColor(LIGHTBLUE);
        tft.print("     Cold");
    } else {      
        tft.setTextColor(AZURE);
        tft.print("   Freezing");
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

