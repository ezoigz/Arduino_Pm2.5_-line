#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <PMS.h>

// ====== WiFi + LINE ======
const char* ssid = "ezoizgz";
const char* password = "11111111";
const String token = "Ra7f33KjtYZZy8U24KtQyrJk143Rvd76F6/aQpTOYTXW/ncFdvK9FlMh3bRePHWUBQjegQRGMFyTLBE00B/NIUTV2MvJU9jMmZXeUhpEJf46C5TVo3AYGnbIleZl54qB4yMXJ0OFr3aAvZykQUU5FAdB04t89/1O/w1cDnyilFU=";
const String userId = "U361130d3982d1a868108a97b92cc98e1";

// ====== Plantower PMS ======
HardwareSerial pmsSerial(2); // RX2=16, TX2=17
PMS pms(pmsSerial);
PMS::DATA data;

// ====== PIN ======
const int LED_GREEN  = 25;
const int LED_YELLOW = 26;
const int LED_RED    = 27;
const int BUZZER_PIN = 14;

// Relay pins
const int RELAY1 = 18; // IN1
const int RELAY2 = 19; // IN2 (สำรอง)

// ====== Alarm timing ======
const unsigned long ALARM_INTERVAL_MS = 10000; // 10 วินาที
unsigned long lastAlarmMs = 0;

// รีเลย์ส่วนมากเป็น Active LOW
void relayOn(int pin)  { digitalWrite(pin, LOW); }
void relayOff(int pin) { digitalWrite(pin, HIGH); }

void setLed(bool g, bool y, bool r) {
  digitalWrite(LED_GREEN,  g ? HIGH : LOW);
  digitalWrite(LED_YELLOW, y ? HIGH : LOW);
  digitalWrite(LED_RED,    r ? HIGH : LOW);
}

void allLedsOff() {
  setLed(false, false, false);
}

void beep3Times() {
  for (int i = 0; i < 3; i++) {
    tone(BUZZER_PIN, 2000);
    delay(250);
    noTone(BUZZER_PIN);
    delay(250);
  }
}

void sendLinePush(String text) {
  if (WiFi.status() != WL_CONNECTED) return;

  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient https;
  if (https.begin(client, "https://api.line.me/v2/bot/message/push")) {
    https.addHeader("Content-Type", "application/json");
    https.addHeader("Authorization", "Bearer " + token);

    String body = "{\"to\":\"" + userId + "\",\"messages\":[{\"type\":\"text\",\"text\":\"" + text + "\"}]}";
    int code = https.POST(body);
    Serial.print("LINE push code: ");
    Serial.println(code);
    https.end();
  }
}

void setup() {
  Serial.begin(115200);

  pmsSerial.begin(9600, SERIAL_8N1, 16, 17);

  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_YELLOW, OUTPUT);
  pinMode(LED_RED, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  pinMode(RELAY1, OUTPUT);
  pinMode(RELAY2, OUTPUT);

  allLedsOff();
  relayOff(RELAY1);
  relayOff(RELAY2);

  WiFi.begin(ssid, password);
  Serial.print("Connecting WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");
}

void loop() {
  if (pms.read(data)) {
    int pm25 = data.PM_AE_UG_2_5;
    Serial.print("PM2.5 = ");
    Serial.println(pm25);

    // 0-37 = เขียว, 38-50 = เหลือง, >=51 = แดง
    if (pm25 >= 0 && pm25 <= 37) {
      setLed(true, false, false);
      relayOff(RELAY1);
    } else if (pm25 >= 38 && pm25 <= 50) {
      setLed(false, true, false);
      relayOff(RELAY1);
    } else if (pm25 >= 51) {
      setLed(false, false, true);
      relayOn(RELAY1);
    } else {
      allLedsOff();
      relayOff(RELAY1);
    }

    // เสียง + LINE เมื่อ >=51 ทุก 10 วินาที
    if (pm25 >= 51) {
      unsigned long now = millis();
      if (lastAlarmMs == 0 || now - lastAlarmMs >= ALARM_INTERVAL_MS) {
        beep3Times();
        sendLinePush("ALERT: PM2.5 = " + String(pm25) + " ug/m3 (>=51)");
        lastAlarmMs = now;
      }
    } else {
      lastAlarmMs = 0;
      noTone(BUZZER_PIN);
    }
  }
}
