// CocoaGuard - Complete Simulation Code with LED Blink Test
// PIR on pin 2, Buzzer on pin 9, LED on pin 10, Reset on pin 7, Battery on A0

const int pirPin = 2;
const int buzzerPin = 9;
const int ledPin = 10;
const int resetPin = 7;
const int batteryPin = A0;

bool waitingForSecond = false;
bool alarmActive = false;
unsigned long firstPressTime = 0;
const unsigned long windowMs = 5000;          // 5 seconds to wait for second press

unsigned long alarmStartTime = 0;
const unsigned long alarmDurationMs = 10000;  // Alarm stays on for 10 seconds

unsigned long lastBatteryRead = 0;
const unsigned long batteryIntervalMs = 60000; // Read battery every 60 seconds

void setup() {
  pinMode(pirPin, INPUT);
  pinMode(buzzerPin, OUTPUT);
  pinMode(ledPin, OUTPUT);
  pinMode(resetPin, INPUT_PULLUP);
  pinMode(batteryPin, INPUT);
  
  digitalWrite(ledPin, LOW);
  noTone(buzzerPin);
  
  Serial.begin(9600);
  delay(2000);                     // Wait for Serial Monitor to connect
  Serial.println();                // Clear any partial buffer
  
  // --- LED Blink Test at startup (3 times) ---
  for (int i = 0; i < 3; i++) {
    digitalWrite(ledPin, HIGH);
    Serial.println("LED ON (startup test)");
    delay(500);
    digitalWrite(ledPin, LOW);
    Serial.println("LED OFF");
    delay(500);
  }
  
  // Print CSV header
  Serial.println("timestamp_ms,event_type,sensor_id,value,alert_sent,sms_status,battery_voltage");
  Serial.println("0,SYSTEM_START,MAIN,1,FALSE,,");
}

void loop() {
  unsigned long now = millis();
  int pirState = digitalRead(pirPin);       // HIGH = motion detected
  int resetState = digitalRead(resetPin);   // LOW = button pressed
  int rawBattery = analogRead(batteryPin);
  float batteryVoltage = rawBattery * (6.6 / 1023.0);  // Simulate 0-6.6V battery range
  
  // --- Battery reading every 60 seconds ---
  if (now - lastBatteryRead >= batteryIntervalMs) {
    lastBatteryRead = now;
    Serial.print(now);
    Serial.print(",BATTERY_READ,BATTERY,");
    Serial.print(batteryVoltage);
    Serial.print(",FALSE,,");
    Serial.println(batteryVoltage);
    
    if (batteryVoltage < 4.5) {
      Serial.print(now);
      Serial.println(",LOW_BATTERY_ALERT,BATTERY,1,TRUE,FAIL,");
    }
  }
  
  // --- Manual reset (button pressed) ---
  if (resetState == LOW) {
    waitingForSecond = false;
    alarmActive = false;
    digitalWrite(ledPin, LOW);
    noTone(buzzerPin);
    Serial.print(now);
    Serial.println(",SYSTEM_RESET,MAIN,1,TRUE,,");
    delay(250);  // debounce
  }
  
  // --- Auto reset after alarm duration ---
  if (alarmActive && (now - alarmStartTime >= alarmDurationMs)) {
    alarmActive = false;
    waitingForSecond = false;
    digitalWrite(ledPin, LOW);
    noTone(buzzerPin);
    Serial.print(now);
    Serial.println(",ALARM_AUTO_RESET,MAIN,1,FALSE,,");
  }
  
  // --- If alarm is active, ignore PIR (no new detections until reset) ---
  if (alarmActive) return;
  
  // --- Two-press false‑alarm filter ---
  if (pirState == HIGH && !waitingForSecond) {
    // First detection
    waitingForSecond = true;
    firstPressTime = now;
    Serial.print(now);
    Serial.println(",MOTION_START,PIR1,1,FALSE,,");
    Serial.print(now);
    Serial.println(",FIRST_DETECTION,PIR1,1,FALSE,,");
  }
  else if (pirState == HIGH && waitingForSecond && (now - firstPressTime <= windowMs)) {
    // Second detection within window -> real intrusion
    alarmActive = true;
    alarmStartTime = now;
    waitingForSecond = false;
    digitalWrite(ledPin, HIGH);
    tone(buzzerPin, 2000);
    Serial.print(now);
    Serial.println(",INTRUSION_CONFIRMED,PIR1,1,TRUE,,");
    Serial.print(now);
    Serial.println(",SMS_SENT,GSM,1,TRUE,SUCCESS,");
  }
  else if (waitingForSecond && (now - firstPressTime > windowMs)) {
    // Window expired without second press -> false alarm
    waitingForSecond = false;
    Serial.print(now);
    Serial.println(",FALSE_ALARM,PIR1,0,FALSE,,");
  }
  
  delay(50);
}