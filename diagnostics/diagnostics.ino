/*
 * OLED Test - Mimics main code initialization exactly
 * This tests whether the main code's OLED initialization works
 */

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Same settings as main code
#define OLED_ADDRESS 0x3C
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

void setup() {
  // Initialize Serial for debugging
  Serial.begin(115200);
  while (!Serial);  // Wait for Serial Monitor
  Serial.println("=== OLED Test (Main Code Like) ===");

  // Initialize pins (same as main)
  pinMode(3, INPUT_PULLUP);  // JOY_BUTTON_PIN
  pinMode(9, OUTPUT);        // BUZZER_PIN
  Serial.println("Pins initialized");

  // Initialize OLED (EXACTLY like main code)
  Serial.print("Initializing OLED at address 0x");
  Serial.println(OLED_ADDRESS, HEX);
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) {
    Serial.println("OLED INIT FAILED - Entering infinite loop");
    // Handle error - for now, just loop forever (same as main code)
    while (true)
      ;
  }
  Serial.println("OLED INIT SUCCESS");

  display.clearDisplay();
  display.display();
  Serial.println("Display cleared and displayed");

  // Simulate loadPet() delay
  delay(100);
  Serial.println("Delay completed");

  // Test drawing something
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(10, 20);
  display.print("TEST");
  display.display();
  Serial.println("Test text drawn");

  Serial.println("Setup completed successfully!");
}

void loop() {
  // Blink LED to show we're alive
  static unsigned long lastBlink = 0;
  if (millis() - lastBlink > 1000) {
    lastBlink = millis();
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));

    Serial.print("Loop running, millis: ");
    Serial.println(millis());
  }

  delay(100);
}