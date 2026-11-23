#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#define PIN_ECG 34
#define PIN_LO_P 32
#define PIN_LO_N 33
#define LED_BPM 25
#define LED_LEAD 26
#define LED_SYSTEM 27
#define OLED_SDA 21
#define OLED_SCL 22

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1

Adafruit_SSD1306 oled(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

const uint32_t SAMPLE_INTERVAL_US = 7812;
const uint32_t OLED_UPDATE_INTERVAL_MS = 1000;
const uint16_t PEAK_THRESHOLD = 2500;

uint32_t nextSampleTime = 0;
uint32_t lastOLEDUpdate = 0;
uint32_t lastBeatTime = 0;

int bpm = 0;
int previousValue = 0;
bool beatDetected = false;
bool firstBeat = true;

char classification[12] = "WAITING";
float confidence = 0.0f;
char serialBuffer[40];
size_t bufferIndex = 0;
bool discardSerialLine = false;

bool validLabel(const char *label) {
  return strcmp(label, "WAITING") == 0 ||
         strcmp(label, "CHF") == 0 ||
         strcmp(label, "NSR") == 0;
}

void parseClassificationLine() {
  serialBuffer[bufferIndex] = '\0';
  char *separator = strchr(serialBuffer, ',');
  if (separator == nullptr) {
    return;
  }

  *separator = '\0';
  char *probabilityText = separator + 1;
  char *end = nullptr;
  float newConfidence = strtof(probabilityText, &end);

  if (!validLabel(serialBuffer) ||
      end == probabilityText ||
      *end != '\0' ||
      !isfinite(newConfidence) ||
      newConfidence < 0.0f ||
      newConfidence > 1.0f) {
    return;
  }

  strncpy(classification, serialBuffer, sizeof(classification) - 1);
  classification[sizeof(classification) - 1] = '\0';
  confidence = newConfidence;
}

void readClassification() {
  while (Serial.available() > 0) {
    const char incoming = static_cast<char>(Serial.read());

    if (incoming == '\n') {
      if (!discardSerialLine) {
        parseClassificationLine();
      }
      bufferIndex = 0;
      discardSerialLine = false;
      continue;
    }

    if (incoming == '\r' || discardSerialLine) {
      continue;
    }

    if (bufferIndex < sizeof(serialBuffer) - 1) {
      serialBuffer[bufferIndex++] = incoming;
    } else {
      bufferIndex = 0;
      discardSerialLine = true;
    }
  }
}

void detectBPM(int ecgValue, bool leadsOff, uint32_t now) {
  if (leadsOff) {
    bpm = 0;
    lastBeatTime = 0;
    beatDetected = false;
    firstBeat = true;
    previousValue = ecgValue;
    return;
  }

  if (!beatDetected &&
      previousValue < PEAK_THRESHOLD &&
      ecgValue >= PEAK_THRESHOLD &&
      now - lastBeatTime > 300) {
    if (!firstBeat) {
      const uint32_t interval = now - lastBeatTime;
      const int newBpm = static_cast<int>(60000UL / interval);
      if (newBpm >= 30 && newBpm <= 220) {
        bpm = newBpm;
      }
    }
    lastBeatTime = now;
    firstBeat = false;
    beatDetected = true;
  }

  if (ecgValue < PEAK_THRESHOLD - 100) {
    beatDetected = false;
  }
  previousValue = ecgValue;
}

void updateOLED(bool leadsOff, uint32_t ecgMv) {
  oled.clearDisplay();
  oled.setTextColor(SSD1306_WHITE);
  oled.setTextSize(1);
  oled.setCursor(0, 0);
  oled.println("ECG MONITOR");
  oled.drawLine(0, 10, 127, 10, SSD1306_WHITE);

  oled.setCursor(0, 13);
  oled.println(leadsOff ? "LEAD: OFF" : "LEAD: CONNECTED");
  oled.setCursor(0, 23);
  oled.print("BPM: ");
  if (bpm > 0 && !leadsOff) {
    oled.println(bpm);
  } else {
    oled.println("--");
  }

  oled.setCursor(0, 33);
  oled.print("CLASS: ");
  oled.println(classification);
  oled.setCursor(0, 43);
  oled.print("CONF: ");
  oled.print(confidence * 100.0f, 1);
  oled.println("%");
  oled.setCursor(0, 53);
  oled.print("ADC: ");
  oled.print(ecgMv);
  oled.println(" mV");
  oled.display();
}

void setup() {
  Serial.begin(115200);
  pinMode(PIN_LO_P, INPUT);
  pinMode(PIN_LO_N, INPUT);
  pinMode(LED_BPM, OUTPUT);
  pinMode(LED_LEAD, OUTPUT);
  pinMode(LED_SYSTEM, OUTPUT);

  analogReadResolution(12);
  analogSetPinAttenuation(PIN_ECG, ADC_11db);
  Wire.begin(OLED_SDA, OLED_SCL);
  Wire.setClock(400000);

  if (!oled.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    digitalWrite(LED_SYSTEM, LOW);
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
  oled.println("SAMPLING: 128 Hz");
  oled.display();

  nextSampleTime = micros();
  lastOLEDUpdate = millis();
}

void loop() {
  readClassification();

  const uint32_t currentMicros = micros();
  if (static_cast<int32_t>(currentMicros - nextSampleTime) >= 0) {
    nextSampleTime += SAMPLE_INTERVAL_US;
    if (static_cast<int32_t>(currentMicros - nextSampleTime) >= 0) {
      nextSampleTime = currentMicros + SAMPLE_INTERVAL_US;
    }

    const bool leadsOff =
        digitalRead(PIN_LO_P) == HIGH || digitalRead(PIN_LO_N) == HIGH;
    const int ecgValue = analogRead(PIN_ECG);
    const uint32_t sampleTime = micros();
    const uint32_t nowMillis = millis();

    detectBPM(ecgValue, leadsOff, nowMillis);
    digitalWrite(LED_LEAD, leadsOff ? HIGH : LOW);
    digitalWrite(LED_BPM, (!leadsOff && beatDetected) ? HIGH : LOW);
    digitalWrite(LED_SYSTEM, HIGH);

    Serial.print(sampleTime);
    Serial.print(',');
    if (leadsOff) {
      Serial.println("LEAD_OFF");
    } else {
      Serial.println(ecgValue);
    }
  }

  const uint32_t nowMillis = millis();
  if (nowMillis - lastOLEDUpdate >= OLED_UPDATE_INTERVAL_MS) {
    lastOLEDUpdate = nowMillis;
    const bool leadsOff =
        digitalRead(PIN_LO_P) == HIGH || digitalRead(PIN_LO_N) == HIGH;
    const uint32_t ecgMv = analogReadMilliVolts(PIN_ECG);
    updateOLED(leadsOff, ecgMv);
  }
}
