// Uhr Sketch vom 11.02 


#include <Wire.h>
#include <RTClib.h>
#include <Adafruit_NeoPixel.h>
#include <TM1637Display.h>

#define CLK 2
#define DIO 3
#define BUTTON_PIN A1
#define NEOPIXEL_PIN 11
#define NEOPIXEL_PIN2 10
#define NUMPIXELS 16

RTC_DS1307 rtc;
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel pixels2 = Adafruit_NeoPixel(NUMPIXELS, NEOPIXEL_PIN2, NEO_GRB + NEO_KHZ800);
TM1637Display display(CLK, DIO);

bool buttonState = HIGH;
bool lastButtonState = HIGH;
bool ring2State = false;
unsigned long buttonPressTime = 0;

enum Mode {
  NORMAL,
  SET_HOUR,
  SET_MINUTE
};

Mode mode = NORMAL;
int setHour = 0;
int setMinute = 0;

void setup() {
  Serial.begin(9600);
  Wire.begin();
  pixels.begin();
  pixels.setBrightness(50);
  pixels2.begin();
  pixels2.setBrightness(50);
  display.setBrightness(0x01);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);
  }

  if (!rtc.isrunning()) {
    Serial.println("RTC is NOT running!");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
}

void loop() {
  DateTime now = rtc.now();

  int stunde = now.hour();
  int minute = now.minute();
  int sekunde = now.second();

  bool blink = sekunde % 2 == 0;

  if (mode == SET_HOUR) {
    stunde = setHour;
    minute = -1;
  } else if (mode == SET_MINUTE) {
    stunde = -1;
    minute = setMinute;
  }

  Serial.print(stunde);
  Serial.print(":");
  Serial.print(minute);
  Serial.print(":");
  Serial.println(sekunde);

  showTime(stunde, minute, blink);

  setNeoPixelColorByTime(stunde);

  buttonState = digitalRead(BUTTON_PIN);
  if (buttonState != lastButtonState) {
    if (buttonState == LOW) {
      buttonPressTime = millis();
    } else {
      unsigned long buttonPressDuration = millis() - buttonPressTime;
      if (buttonPressDuration < 2000) {
        if (mode == NORMAL) {
          ring2State = !ring2State;
          setNeoPixelColor(pixels2, ring2State ? pixels2.Color(255, 130, 0) : pixels2.Color(0, 0, 0));
          pixels2.show();
        } else if (mode == SET_HOUR) {
          setHour = (setHour + 1) % 24;
        } else if (mode == SET_MINUTE) {
          setMinute = (setMinute + 1) % 60;
        }
      } else if (buttonPressDuration >= 5000) {
        if (mode == NORMAL) {
          mode = SET_HOUR;
          setHour = stunde;
          setMinute = minute;
        } else if (mode == SET_HOUR) {
          mode = SET_MINUTE;
        } else if (mode == SET_MINUTE) {
          mode = NORMAL;
          rtc.adjust(DateTime(now.year(), now.month(), now.day(), setHour, setMinute, 0));
        }
      }
      buttonPressTime = 0;
    }
    lastButtonState = buttonState;
  }

  delay(500);
}

void showTime(int stunde, int minute, bool blink) {
  uint8_t data[] = { 0x00, 0x00, 0x00, 0x00 };

  if (stunde != -1) {
    data[0] = display.encodeDigit(stunde / 10);
    data[1] = display.encodeDigit(stunde % 10);
  }

  if (minute != -1) {
    data[2] = display.encodeDigit(minute / 10);
    data[3] = display.encodeDigit(minute % 10);
  }

  if (blink && mode == NORMAL) {
    data[1] |= 0x80; // blinkender Punkt zwischen Stunden und Minuten
  }

  display.setSegments(data);
}

void setNeoPixelColorByTime(int stunde) {
  if (stunde >= 19 || stunde < 6) {
    setNeoPixelColor(pixels, pixels.Color(255, 130, 0));
  } else if (stunde >= 6 && stunde < 7) {
    setNeoPixelColor(pixels, pixels.Color(255, 50, 0));
  } else if (stunde >= 7 && stunde < 8) {
    setNeoPixelColor(pixels, pixels.Color(0, 255, 0));    
  } else {
    setNeoPixelColor(pixels, pixels.Color(0, 0, 0));
  }
}

void setNeoPixelColor(Adafruit_NeoPixel &pixels, uint32_t color) {
  for (int i = 0; i < NUMPIXELS; i++) {
    pixels.setPixelColor(i, color);
  }
  pixels.show();
}
