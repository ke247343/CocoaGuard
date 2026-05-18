// CocoaGuard - Advanced Multi-Zone Security Logic with Day/Night Filter
// Inputs: PIR Zone A (Pin 2), PIR Zone B (Pin 3), Reset Button (Pin 7)
// Analog: Battery Voltage (Pin A0 via Divider), LDR Sensor (Pin A1)
// Outputs: Piezo Buzzer (Pin 9), Strobe LED (Pin 10)

const int pirPinA = 2;
const int pirPinB = 3;
const int buzzerPin = 9;
const int ledPin = 10;
const int resetPin = 7;
const int batteryPin = A0;
const int ldrPin = A1;

// TO DO: State Tracking Variables
int lastPirStateA = LOW;
int lastPirStateB = LOW;
bool waitingForSecond = false;
bool alarmActive = false;

//TO DO: 5-second multi-zone confirmation window, 10-second local alarm duration, 10-second SMS network cooldown
unsigned long firstPressTime = 0;
const unsigned long windowMs = 5000;
unsigned long alarmStartTime = 0;
const unsigned long alarmDurationMs = 10000;
unsigned long lastSmsTime = 0;
const unsigned long cooldownMs = 10000;

//TO DO: 60-second battery checking interval
unsigned long lastBatteryRead = 0;
const unsigned long batteryIntervalMs = 60000;

//TO DO: LDR Threshold for Day/Night calculation (0 = dark, 1023 = bright)
const int nighthreshold = 400;

void setup() {
  pinMode(pirPinA, INPUT);
  pinMode(pirPinB, INPUT);
  pinMode(buzzerPin, OUTPUT);
  pinMode(ledPin, OUTPUT);
  pinMode(resetPin, INPUT_PULLUP);
  pinMode(batteryPin, INPUT);
  pinMode(ldrPin, INPUT);

  digitalWrite(ledPin, LOW);
  noTone(buzzerPin);

  Serial.begin(9600);
  delay(2000);
  Serial.println();

  // LED Strobe Startup Test
  for (int i = 0; i < 3; i++) {
    digitalWrite(ledPin, HIGH);
    delay(200);
    digitalWrite(ledPin, LOW);
    delay(200);
  }

  // Print CSV Schema Header to output buffer
  Serial.println("timestamp_ms,event_type,sensor_id,value,alert_sent,sms_status,battery_voltage");
  Serial.println("0,SYSTEM_START,MAIN,1,FALSE,,");
}

void loop() {
  unsigned long now = millis();

  // Read digital inputs
  int pirStateA = digitalRead(pirPinA);
  int pirStateB = digitalRead(pirPinB);
  int resetState = digitalRead(resetPin);

  // Read analog inputs
  int rawBattery = analogRead(batteryPin);
  int ldrValue = analogRead(ldrPin);

  // Hardware Voltage Divider Calculation:
  // Converts 0-5V ADC exposure back to the true 0-15.6V scale of a 12V system via a 10k/4.7k divider network
  float measuredVout = rawBattery * (5.0 / 1023.0);
  float batteryVoltage = measuredVout * ((10000.0 + 4700.0) / 4700.0);

  //TO DO: Battery Evaluation Task
  if (now - lastBatteryRead >= batteryIntervalMs) {
    lastBatteryRead = now;
    Serial.print(now);
    Serial.print(",BATTERY_READ,BATTERY,");
    Serial.print(batteryVoltage);
    Serial.print(",FALSE,,");
    Serial.println(batteryVoltage);

    // Low voltage warning threshold for 12V SLA Battery
    if (batteryVoltage < 11.1) {
      Serial.print(now);
      Serial.print(",LOW_BATTERY_ALERT,BATTERY,1,TRUE,FAIL,");
      Serial.println(batteryVoltage);
    }
  }

  // TO DO: Manual Switch Reset Override
  if (resetState == LOW) {
    waitingForSecond = false;
    alarmActive = false;
    digitalWrite(ledPin, LOW);
    noTone(buzzerPin);
    Serial.print(now);
    Serial.println(",SYSTEM_RESET,MAIN,1,TRUE,,");

    // Hard switch debounce
    delay(250);
  }

  // TO DO: Automatic Timeout Deterrence Reset
  if (alarmActive && (now - alarmStartTime >= alarmDurationMs)) {
    alarmActive = false;
    waitingForSecond = false;
    digitalWrite(ledPin, LOW);
    noTone(buzzerPin);
    Serial.print(now);
    Serial.println(",ALARM_AUTO_RESET,MAIN,1,FALSE,,");
  }

  // TO DO: Unified Multi-Zone Edge Transition Detection
  bool newMotionTriggered = false;
  String activeZone = "";

  if (pirStateA == HIGH && lastPirStateA == LOW) {
    newMotionTriggered = true;
    activeZone = "PIR_ZONE_A";
  } else if (pirStateB == HIGH && lastPirStateB == LOW) {
    newMotionTriggered = true;
    activeZone = "PIR_ZONE_B";
  }

  if (newMotionTriggered) {
    if (alarmActive) {
      // Monitor and record secondary triggers during active deterrence cooldown windows
      Serial.print(now);
      Serial.print(",ALERT_BLOCKED,");
      Serial.print(activeZone);
      Serial.println(",1,FALSE,COOLDOWN,");
    }
    else if (!waitingForSecond) {
      // First verification state entry
      waitingForSecond = true;
      firstPressTime = now;
      Serial.print(now);
      Serial.print(",MOTION_START,");
      Serial.print(activeZone);
      Serial.println(",1,FALSE,,");

      Serial.print(now);
      Serial.print(",FIRST_DETECTION,");
      Serial.print(activeZone);
      Serial.println(",1,FALSE,,");
    }
    else if (waitingForSecond && (now - firstPressTime <= windowMs)) {
      // Second entry point validated within rolling 5-second filter window
      waitingForSecond = false;
      alarmActive = true;
      alarmStartTime = now;

      // Local Hardware Deterrence Optimization based on LDR Light Sensor Reading
      tone(buzzerPin, 2000); // Buzzer runs regardless of time of day

      if (ldrValue < nighthreshold) {
        // It is dark out: Activate both the high-frequency buzzer and the flashing visual strobe LED
        digitalWrite(ledPin, HIGH);
      } else {
        // It is daytime: Deactivate strobe light path to maximize solar battery economy
        digitalWrite(ledPin, LOW);
      }

      Serial.print(now);
      Serial.print(",INTRUSION_CONFIRMED,");
      Serial.print(activeZone);
      Serial.println(",1,TRUE,,");

      // TO DO: Network Transmission Cooldown Gate
      if (lastSmsTime == 0 || (now - lastSmsTime >= cooldownMs)) {
        lastSmsTime = now;
        Serial.print(now);
        Serial.println(",SMS_SENT,GSM,1,TRUE,SUCCESS,");
      } else {
        Serial.print(now);
        Serial.println(",ALERT_BLOCKED,GSM,1,FALSE,COOLDOWN,");
      }
    }
  }

  // TO DO: Rolling Window False Alarm Cleanup
  if (waitingForSecond && (now - firstPressTime > windowMs)) {
    waitingForSecond = false;
    Serial.print(now);
    Serial.println(",FALSE_ALARM,MAIN,0,FALSE,,");
  }

  // TO DO: Commit state histories for the subsequent evaluation loops
  lastPirStateA = pirStateA;
  lastPirStateB = pirStateB;

  delay(50);
}
