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

// Definieren Sie die Zeitpunkte für die NeoPixel-Farben
#define NIGHT_START_HOUR 19
#define NIGHT_END_HOUR 6
#define MORNING_START_HOUR 6
#define MORNING_END_HOUR 7
#define DAY_START_HOUR 7
#define DAY_END_HOUR 8

// Einstellen der Farben für die Uhrezeiten, (ROT, GRÜN, BLAU) von 0 aus bis 255 maximale Helligkeit
#define COLOR_NIGHT (255, 130, 0)
#define COLOR_MORNING (255, 50, 0)
#define COLOR_DAY (0, 255, 0)

RTC_DS1307 rtc;
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel pixels2 = Adafruit_NeoPixel(NUMPIXELS, NEOPIXEL_PIN2, NEO_GRB + NEO_KHZ800);
TM1637Display display(CLK, DIO);

bool buttonState = HIGH;
bool lastButtonState = HIGH;
bool ring2State = false;
unsigned long buttonPressTime = 0;
bool longPressHandled = false;
unsigned long lastInputTime = 0; // Zeit der letzten Eingabe

enum Mode {
  NORMAL,
  SET_BRIGHTNESS,
  SET_HOUR,
  SET_MINUTE
};

Mode mode = NORMAL;
int setHour = 0;
int setMinute = 0;
int brightness = 50; // Initial brightness at 50%

void setup() {
  Serial.begin(9600);
  Wire.begin();
  pixels.begin();
  pixels.setBrightness(map(brightness, 0, 100, 0, 255));  // Helligkeit der NeoPixel-Ringe festlegen
  pixels2.begin();
  pixels2.setBrightness(map(brightness, 0, 100, 0, 255)); // Helligkeit des zweiten NeoPixel-Rings festlegen
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

  if (mode == SET_BRIGHTNESS) {
    showBrightness(brightness);
  } else {
    showTime(stunde, minute, blink);
  }

  setNeoPixelColorByTime(stunde);

  buttonState = digitalRead(BUTTON_PIN);
  if (buttonState == LOW && lastButtonState == HIGH) {
    buttonPressTime = millis();
    longPressHandled = false;
    lastInputTime = millis(); // Zeit der letzten Eingabe aktualisieren
  } else if (buttonState == HIGH && lastButtonState == LOW) {
    unsigned long buttonPressDuration = millis() - buttonPressTime;
    if (buttonPressDuration < 2000 && !longPressHandled) {
      if (mode == NORMAL) {
        ring2State = !ring2State;
        setNeoPixelColor(pixels2, ring2State ? pixels2.Color(255, 130, 0) : pixels2.Color(0, 0, 0));
        pixels2.show();
      } else if (mode == SET_BRIGHTNESS) {
        brightness = (brightness + 5) % 105;
        if (brightness == 0) brightness = 5;
        pixels.setBrightness(map(brightness, 0, 100, 0, 255));
        pixels2.setBrightness(map(brightness, 0, 100, 0, 255));
        pixels.show();
        pixels2.show();
      } else if (mode == SET_HOUR) {
        setHour = (setHour + 1) % 24;
      } else if (mode == SET_MINUTE) {
        setMinute = (setMinute + 1) % 60;
      }
    }
    buttonPressTime = 0;
  }

  // Check if button is held down for more than 5 seconds
  if (buttonState == LOW && (millis() - buttonPressTime) >= 5000 && !longPressHandled) {
    if (mode == NORMAL) {
      mode = SET_BRIGHTNESS;
    } else if (mode == SET_BRIGHTNESS) {
      mode = SET_HOUR;
      setHour = stunde;
      setMinute = minute;
    } else if (mode == SET_HOUR) {
      mode = SET_MINUTE;
    } else if (mode == SET_MINUTE) {
      mode = NORMAL;
      rtc.adjust(DateTime(now.year(), now.month(), now.day(), setHour, setMinute, 0));
    }
    longPressHandled = true; // Mark long press as handled
    buttonPressTime = millis(); // Reset button press time to avoid multiple mode changes
  }

  lastButtonState = buttonState;

  // Überprüfen, ob seit der letzten Eingabe mehr als 10 Sekunden vergangen sind
  if (mode != NORMAL && millis() - lastInputTime >= 15000) {
    mode = NORMAL; // Setzen Sie den Modus auf NORMAL zurück
  }

  delay(50); // Reduzierte Verzögerung für genauere Zeitüberwachung
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

void showBrightness(int brightness) {
  uint8_t data[] = { 0x00, 0x00, 0x00, 0x00 };

  int brightnessPercentage = brightness;
  data[3] = display.encodeDigit(brightnessPercentage % 10); // Einheiten
  data[2] = display.encodeDigit((brightnessPercentage / 10) % 10); // Zehner
  data[1] = display.encodeDigit((brightnessPercentage / 100) % 10); // Hunderter
  data[0] = display.encodeDigit(0); // Stellenwert für Tausender bleibt 0

  display.setSegments(data);
}

void setNeoPixelColorByTime(int stunde) {
  if (stunde >= NIGHT_START_HOUR || stunde < NIGHT_END_HOUR) {
    setNeoPixelColor(pixels, pixels.Color COLOR_NIGHT);
  } else if (stunde >= MORNING_START_HOUR && stunde < MORNING_END_HOUR) {
    setNeoPixelColor(pixels, pixels.Color COLOR_MORNING);
  } else if (stunde >= DAY_START_HOUR && stunde < DAY_END_HOUR) {
    setNeoPixelColor(pixels, pixels.Color COLOR_DAY);    
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
