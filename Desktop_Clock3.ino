
/**************************************************************************
Wemos D1 mini

Touch sensor
GPIO 0 (D3) attachInterrupt

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
#include <Adafruit_ST7789.h> // Hardware-specific library for ST7789
#include <SPI.h>
#include <Wire.h>
#include <DS3231.h>
#include <Ticker.h>

#define TFT_PIN_CS   15
#define TFT_PIN_DC   2
#define TFT_PIN_RST  12
#define TFT_BACKLIGHT  16 // Display backlight pin

volatile int state = HIGH;
Ticker ticker;
volatile int state2 = HIGH;

int sig_pin = 0; // Touch sensor
int cdsVal = 0;

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_PIN_CS, TFT_PIN_DC, TFT_PIN_RST);


float p = 3.1415926;
char deg_sym = (char) 176;

DS3231 clock;
RTCDateTime dt;

void setup(void) {
  Serial.begin(9600);
  pinMode(sig_pin,INPUT);
  attachInterrupt(sig_pin,displayOnOff,RISING);
  ticker.attach(30, readTempTimer);
  // Initializer if using a 1.8" TFT screen:
  pinMode(TFT_BACKLIGHT, OUTPUT);
  digitalWrite(TFT_BACKLIGHT, state); // Backlight on
  tft.initR(INITR_BLACKTAB);      // Init ST7735S chip, black tab
  Serial.println(F("1.8 TFT screen Initialized"));
  tft.setRotation(1); //Rotate Display 0=0, 1=90, 2=180, 3=240
  tft.fillScreen(ST77XX_BLACK);
 
  // Initialize DS3231
  Serial.println("Initialize DS3231");
  clock.begin();
  // Set sketch compiling time
//clock.setDateTime(__DATE__, __TIME__);
}

void loop() {
  Serial.print("backlight state = ");
  Serial.println(state);
  
  digitalWrite(TFT_BACKLIGHT, state);
  delay(1000);


  if (state2){
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextColor(ST77XX_WHITE);
  tft.drawRoundRect(15,35,135,40,10,ST77XX_WHITE);
  
  dt = clock.getDateTime();
  // For leading zero look to DS3231_dateformat example
  Serial.print("Raw data: ");
  Serial.print(dt.year);   Serial.print("-");
  Serial.print(dt.month);  Serial.print("-");
  Serial.print(dt.day);    Serial.print(" ");
  Serial.print(dt.hour);   Serial.print(":");
  Serial.print(dt.minute); Serial.print(":");
  Serial.print(dt.second); Serial.println("");
   
  Serial.print("Temperature: "); 
  Serial.print(clock.readTemperature());
  Serial.println(" *C ");

  tft.setCursor(0, 0);
  tft.setTextSize(2);
  tft.print(dt.year);
  tft.print('/');
  tft.print(dt.month);
  tft.print('/');
  tft.print(dt.day);

  tft.setCursor(20, 120);
  tft.setTextSize(1);
  tft.print("Temperature : ");
  tft.print(clock.readTemperature());
//  tft.print(deg_sym);
  tft.println(" C");
  
  tft.setCursor(0,40);
  tft.setTextSize(4);
  tft.print(" ");
  tft.print(dt.hour);
  tft.print(':');
  tft.print(dt.minute);

  cdsVal = analogRead(A0); // input CDS sensor value
  Serial.print("Lighting value : "); 
  Serial.print(cdsVal);
  tft.setCursor(20, 110);
  tft.setTextSize(1);
  tft.print("Lighting value : ");
  tft.print(cdsVal);

  tft.setCursor(0, 80);
  tft.setTextSize(2);
  if (clock.readTemperature() > 35){
     tft.setTextColor(ST77XX_RED);
     tft.print("   Hot !!!");
  } else if (clock.readTemperature() > 29){
     tft.setTextColor(ST77XX_RED);
     tft.print("    Warm");
  } else if (clock.readTemperature() > 21){
      tft.setTextColor(ST77XX_GREEN);
      tft.print(" Comfortable");
  } else if (clock.readTemperature() > 15){
      tft.setTextColor(ST77XX_BLUE);
      tft.print("     Cool");
  } else if (clock.readTemperature() > 1){
      tft.setTextColor(ST77XX_BLUE);
      tft.print("     Cold");
  } else {      
      tft.setTextColor(ST77XX_BLUE);
      tft.print("   Freezing");
  }
  
  state2 = !state2;
  }
  
// Serial.println(digitalRead(sig_pin));
//  if(digitalRead(sig_pin))
//  {
//
//     tft.setCursor(0, 80);
//     tft.setTextColor(ST77XX_RED);
//     tft.setTextSize(2);
//     tft.print("Switch ON ! ");
//     displayOnOff();
//     digitalWrite(TFT_BACKLIGHT, state); // Backlight on
//     
//  }
//  else
//  {
//
//     tft.setCursor(0, 80);
//     tft.setTextColor(ST77XX_GREEN);
//     tft.setTextSize(2);
//     tft.print("Switch OFF");
//
//    }

  
//  tft.invertDisplay(true);
//  delay(500);
//  tft.invertDisplay(false);
//  delay(500);
}

void displayOnOff()
{
  state = !state;
}

void readTempTimer()
{
 state2 = !state2;
}

