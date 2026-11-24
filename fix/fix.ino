#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// PIN AD8232
#define PIN_ECG 34
#define PIN_LO_P 32
#define PIN_LO_N 33

// PIN LED
#define LED_BPM 25
#define LED_LEAD 26
#define LED_SYSTEM 27

// PIN OLED
#define OLED_SDA 21
#define OLED_SCL 22

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1

Adafruit_SSD1306 oled(
  SCREEN_WIDTH,
  SCREEN_HEIGHT,
  &Wire,
  OLED_RESET
);

// BPM
unsigned long lastBeatTime = 0;
unsigned long lastOLEDUpdate = 0;

int bpm = 0;
int previousValue = 0;

bool beatDetected = false;
bool firstBeat = true;

const int peakThreshold = 2500;

void setup() {
  Serial.begin(115200);

  pinMode(PIN_LO_P, INPUT);
  pinMode(PIN_LO_N, INPUT);

  // LED SETUP
  pinMode(LED_BPM, OUTPUT);
  pinMode(LED_LEAD, OUTPUT);
  pinMode(LED_SYSTEM, OUTPUT);

  digitalWrite(LED_BPM, LOW);
  digitalWrite(LED_LEAD, LOW);
  digitalWrite(LED_SYSTEM, HIGH);

  analogReadResolution(12);
  analogSetPinAttenuation(PIN_ECG, ADC_11db);

  Wire.begin(OLED_SDA, OLED_SCL);

  if (!oled.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    while (true) {
      delay(100);
    }
  }

  oled.clearDisplay();
  oled.setTextColor(SSD1306_WHITE);
  oled.setTextSize(1);
  oled.setCursor(0, 0);
  oled.println("ECG MONITOR");
  oled.println("ESP32 READY");
  oled.display();

  delay(1000);
}

void loop() {
  int ecgValue = analogRead(PIN_ECG);

  int loPlus = digitalRead(PIN_LO_P);
  int loMinus = digitalRead(PIN_LO_N);

  bool leadsOff = (loPlus == HIGH || loMinus == HIGH);

  uint32_t ecgMv = analogReadMilliVolts(PIN_ECG);

  unsigned long now = millis();

  // DETEKSI BPM SEDERHANA
  if (leadsOff) {
    bpm = 0;
    lastBeatTime = 0;
    beatDetected = false;
    firstBeat = true;
  } else {
    if (!beatDetected &&
        previousValue < peakThreshold &&
        ecgValue >= peakThreshold &&
        now - lastBeatTime > 300) {

      if (!firstBeat) {
        unsigned long interval = now - lastBeatTime;
        int newBpm = 60000 / interval;

        if (newBpm >= 30 && newBpm <= 220) {
          bpm = newBpm;
        }
      }

      lastBeatTime = now;
      firstBeat = false;
      beatDetected = true;
    }

    if (ecgValue < peakThreshold - 100) {
      beatDetected = false;
    }
  }

  previousValue = ecgValue;

  // LED INDICATOR
  digitalWrite(LED_LEAD, leadsOff ? HIGH : LOW);

  digitalWrite(
    LED_BPM,
    (!leadsOff && beatDetected) ? HIGH : LOW
  );

  // SERIAL PLOTTER: ECG MENTAH SAJA
  Serial.println(ecgValue);

  // OLED UPDATE
  if (now - lastOLEDUpdate >= 250) {
    lastOLEDUpdate = now;

    oled.clearDisplay();

    oled.setTextSize(1);
    oled.setCursor(0, 0);

    oled.println("ECG MONITOR");
    oled.drawLine(0, 11, 127, 11, SSD1306_WHITE);

    oled.setCursor(0, 17);

    if (leadsOff) {
      oled.println("LEAD: OFF");
      oled.println("BPM: --");
    } else {
      oled.println("LEAD: CONNECTED");

      oled.print("BPM: ");
      if (bpm > 0) {
        oled.println(bpm);
      } else {
        oled.println("--");
      }
    }

    oled.setCursor(0, 43);
    oled.print("OUTPUT: ");
    oled.print(ecgMv);
    oled.println(" mV");

    oled.display();
  }

  digitalWrite(LED_SYSTEM, HIGH);

  delay(20);
}