/*
  Debug version for testing controls
  Upload this to see what your joystick and button are doing
*/

#define JOY_X_PIN A0
#define JOY_Y_PIN A1
#define JOY_BUTTON_PIN 3

void setup() {
  Serial.begin(115200);
  pinMode(JOY_BUTTON_PIN, INPUT_PULLUP);

  Serial.println(F("=== CONTROL DEBUG TEST ==="));
  Serial.println(F("Move joystick and press button"));
  Serial.println(F("Format: X/Y ButtonState"));
  Serial.println(F("Center should be around 0/0"));
  Serial.println(F("Range: -512 to +512"));
  Serial.println(F("Button: 1=pressed, 0=released"));
  Serial.println(F("=========================="));
}

void loop() {
  // Read joystick
  int joyX = analogRead(JOY_X_PIN) - 512; // Center around 0
  int joyY = analogRead(JOY_Y_PIN) - 512;

  // Read button with debounce
  bool buttonReading = !digitalRead(JOY_BUTTON_PIN); // Active low

  // Print values
  Serial.print(joyX);
  Serial.print(F("/"));
  Serial.print(joyY);
  Serial.print(F(" "));
  Serial.println(buttonReading ? F("PRESSED") : F("released"));

  delay(100);
}