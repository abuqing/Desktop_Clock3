/**************************************************************************
Wemos D1 mini

TFT SPI 128x160 V1.1
GPIO12             Pin 06 (RESET)
GPIO2             Pin 07 (A0)
GPIO13 (HSPID)    Pin 08 (SDA)
GPIO14 (HSPICLK)  Pin 09 (SCK)
GPIO15 (HSPICS)   Pin 10 (CS)

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

#define TFT_PIN_CS   15
#define TFT_PIN_DC   2
#define TFT_PIN_RST  12
#define TFT_BACKLIGHT  0 // Display backlight pin
volatile int state = HIGH;

int sig_pin = 16; // touch sensor
int cdsVal = 0;

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_PIN_CS, TFT_PIN_DC, TFT_PIN_RST);


float p = 3.1415926;
char deg_sym = (char) 176;

DS3231 clock;
RTCDateTime dt;

void setup(void) {
  Serial.begin(9600);

  pinMode(sig_pin,INPUT);
  
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
  
 Serial.println(digitalRead(sig_pin));
  if(digitalRead(sig_pin))
  {

     tft.setCursor(0, 80);
     tft.setTextColor(ST77XX_RED);
     tft.setTextSize(2);
     tft.print("Switch ON ! ");
     change();
     digitalWrite(TFT_BACKLIGHT, state); // Backlight on
     
  }
  else
  {

     tft.setCursor(0, 80);
     tft.setTextColor(ST77XX_GREEN);
     tft.setTextSize(2);
     tft.print("Switch OFF");

    }


 delay(5000);

  
//  tft.invertDisplay(true);
//  delay(500);
//  tft.invertDisplay(false);
//  delay(500);
}

void change()
{
  state = !state;
}
