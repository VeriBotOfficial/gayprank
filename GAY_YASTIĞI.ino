#include <SoftwareSerial.h>
#include <string.h>   // for strstr, memcpy, strtok
#include <stdlib.h>   // for atoi

// HC-06 Bluetooth RX → D10, TX → D11
SoftwareSerial bluetooth(10, 11);

// Flag definitions
const char flag1[] = "kuzey";
const char flag2[] = "berke";
const char flag3[] = "aydin";

// Communication buffer
char buffer[128];
int  counter = 0;

// Timing
uint32_t prevMillis = 0;
const uint32_t interval = 1000;  // 1 second

// Internal clock
int hours   = 0;
int minutes = 0;
int seconds = 0;

// Alarm time
int  alarmHour      = 0;
int  alarmMinute    = 0;
bool alarmSet       = false;
bool alarmTriggered = false;

// Control Bluetooth reading
bool btReadEnabled = true;

// Vibration pin
const uint8_t vibPin = 7;

// Helper: substring search
bool strContains(const char *hay, const char *needle) {
  return strstr(hay, needle) != NULL;
}

void setup() {
  pinMode(vibPin, OUTPUT);
  digitalWrite(vibPin, LOW);

  Serial.begin(9600);
  bluetooth.begin(9600);

  Serial.println(F("Bluetooth test başlatıldı."));
  bluetooth.println(F("Bluetooth test aktif. Merhaba!"));
}

void loop() {
  // 1) Read from Bluetooth only if enabled
  if (btReadEnabled) {
    while (bluetooth.available()) {
      buffer[counter++] = bluetooth.read();
      if (counter >= (int)sizeof(buffer)) {
        counter = 0;  // prevent overflow
      }
    }
  }

  // 2) If alarm not set and we see flag3, parse and set alarm
  if (!alarmSet && strContains(buffer, flag3)) {
    char *p1 = strstr(buffer, flag1);
    char *p2 = strstr(buffer, flag2);
    char *p3 = strstr(buffer, flag3);

    if (p1 && p2 && p3 && p1 < p2 && p2 < p3) {
      // --- first_clock: flag1 → flag2 is "HH:MM:SS"
      p1 += strlen(flag1);
      int fcd = p2 - p1;
      char first_clock[9] = {0};  // max "HH:MM:SS" + '\0'
      if (fcd > 0 && fcd < (int)sizeof(first_clock)) {
        memcpy(first_clock, p1, fcd);
        first_clock[fcd] = '\0';
      }

      // --- second_clock: flag2 → flag3 is "HHqMM"
      p2 += strlen(flag2);
      int scd = p3 - p2;
      char second_clock[6] = {0};  // max "HHqMM" + '\0'
      if (scd > 0 && scd < (int)sizeof(second_clock)) {
        memcpy(second_clock, p2, scd);
        second_clock[scd] = '\0';
      }

      // Print for debugging
      Serial.print(F("first_clock: "));  Serial.println(first_clock);
      Serial.print(F("second_clock: ")); Serial.println(second_clock);

      // Parse first_clock "HH:MM:SS"
      {
        char *tk1 = strtok(first_clock, ":");
        char *tk2 = strtok(NULL,        ":");
        char *tk3 = strtok(NULL,        ":");
        hours   = tk1 ? atoi(tk1) : 0;
        minutes = tk2 ? atoi(tk2) : 0;
        seconds = tk3 ? atoi(tk3) : 0;
      }

      // Parse second_clock "HHqMM"
      {
        char *tk1 = strtok(second_clock, "q");
        char *tk2 = strtok(NULL,         "q");
        alarmHour   = tk1 ? atoi(tk1) : 0;
        alarmMinute = tk2 ? atoi(tk2) : 0;
      }

      // Sync millis‑timer to the parsed seconds
      prevMillis = millis();

      alarmSet = true;
      Serial.print(F("Alarm ayarlandı → "));
      Serial.print(alarmHour);
      Serial.print(':');
      Serial.println(alarmMinute);

      // Stop further Bluetooth reads until after alarm
      btReadEnabled = false;
    }

    // Clear buffer for next read
    memset(buffer, 0, sizeof(buffer));
    counter = 0;
  }

  // 3) Update internal clock (only if alarm is set but not yet triggered)
  if (alarmSet && !alarmTriggered) {
    uint32_t now = millis();
    if (now - prevMillis >= interval) {
      prevMillis += interval;

      // Advance one second
      seconds++;
      if (seconds >= 60) {
        seconds = 0;
        minutes++;
        if (minutes >= 60) {
          minutes = 0;
          hours = (hours + 1) % 24;
        }
      }

      // Optional: print current time
      Serial.print(F("Saat: "));
      if (hours   < 10) Serial.print('0');
      Serial.print(hours);
      Serial.print(':');
      if (minutes < 10) Serial.print('0');
      Serial.print(minutes);
      Serial.print(':');
      if (seconds < 10) Serial.print('0');
      Serial.println(seconds);
    }

    // 4) Check for alarm time
    if (hours == alarmHour && minutes == alarmMinute && seconds == 0) {
      triggerAlarm();
      alarmTriggered = true;
    }
  }
}

// Trigger the vibration pattern, then reset flags to allow new alarms
void triggerAlarm() {
  Serial.println(F("🔔 Alarm zamanı! Titreşiyor..."));

  for (int i = 0; i < 6; i++) {
    digitalWrite(vibPin, HIGH);
    delay(6000);
    digitalWrite(vibPin, LOW);
    delay(1000);
  }
//eğerbunuokuyorsanseninbenamk
  Serial.println(F("Alarm tamamlandı. Bluetooth okuması yeniden başlıyor."));

  // Reset for next cycle
  alarmSet       = false;
  alarmTriggered = false;
  btReadEnabled  = true;
}
