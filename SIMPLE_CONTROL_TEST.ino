/*
  Simplified control test - focuses only on joystick and menu navigation
  Strips away all game logic to isolate control issues
*/

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Pin definitions
#define JOY_X_PIN A0
#define JOY_Y_PIN A1
#define JOY_BUTTON_PIN 3

// OLED settings
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define OLED_ADDRESS 0x3C

// Joystick settings - VERY sensitive for testing
#define JOY_DEADZONE 5  // Very small deadzone
#define DEBOUNCE_DELAY 20

// States
enum { STATE_MAIN, STATE_MENU } currentState;
uint8_t menuSelection = 0;
const char* menuItems[] = {"FOOD", "PLAY", "CLEAN"};
const uint8_t menuItemCount = sizeof(menuItems) / sizeof(menuItems[0]);

// Objects
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Timing
unsigned long lastDebounceTime = 0;
bool joyButtonPressed = false;

void setup() {
  Serial.begin(115200);
  pinMode(JOY_BUTTON_PIN, INPUT_PULLUP);

  // Initialize OLED
  Wire.begin();
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) {
    // Try alternate address
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3D)) {
      Serial.println(F("OLED FAILED"));
    }
  }

  display.clearDisplay();
  display.display();

  currentState = STATE_MAIN;
  Serial.println(F("=== SIMPLE CONTROL TEST ==="));
  Serial.println(F("RIGHT=Menu, LEFT/UP/DOWN=no action for now"));
  Serial.println(F("In Menu: UP/DOWN=navigate, RIGHT/Press=select, LEFT=back"));
}

void loop() {
  // Read inputs
  int joyX = analogRead(JOY_X_PIN) - 512;
  int joyY = analogRead(JOY_Y_PIN) - 512;
  bool buttonReading = !digitalRead(JOY_BUTTON_PIN); // Active low

  // Button debounce
  if (buttonReading && (millis() - lastDebounceTime > DEBOUNCE_DELAY)) {
    joyButtonPressed = true;
    lastDebounceTime = millis();
    Serial.println(F("BUTTON PRESS"));
  } else if (!buttonReading) {
    joyButtonPressed = false;
  }

  // Debug output
  Serial.print(F("Joy: "));
  Serial.print(joyX);
  Serial.print(F("/"));
  Serial.print(joyY);
  Serial.print(F(" Btn: "));
  Serial.println(buttonReading ? "PRESSED" : "released");

  // State machine
  switch (currentState) {
    case STATE_MAIN:
      display.clearDisplay();
      display.setTextSize(2);
      display.setCursor(10, 10);
      display.print(F("MAIN"));
      display.setCursor(10, 30);
      display.print(F("X:"));
      display.print(joyX);
      display.setCursor(10, 50);
      display.print(F("Y:"));
      display.print(joyY);

      // Simple right-joy to open menu
      if (joyX > JOY_DEADZONE) {
        Serial.println(F("-> OPENING MENU"));
        currentState = STATE_MENU;
        delay(200); // Prevent rapid triggers
      }
      break;

    case STATE_MENU:
      display.clearDisplay();

      // Draw menu
      for (uint8_t i = 0; i < menuItemCount; i++) {
        uint8_t y = i * 12;
        if (i == menuSelection) {
          display.fillRect(0, y, SCREEN_WIDTH, 10, SSD1306_WHITE);
          display.setTextColor(SSD1306_BLACK);
        } else {
          display.setTextColor(SSD1306_WHITE);
        }
        display.setCursor(0, y);
        display.print(menuItems[i]);
      }

      // Menu navigation
      if (joyY < -JOY_DEADZONE) {
        menuSelection = (menuSelection + menuItemCount - 1) % menuItemCount;
        Serial.print(F("UP -> "));
        Serial.println(menuSelection);
        delay(150);
      } else if (joyY > JOY_DEADZONE) {
        menuSelection = (menuSelection + 1) % menuItemCount;
        Serial.print(F("DOWN -> "));
        Serial.println(menuSelection);
        delay(150);
      }

      // Menu selection
      if (joyX > JOY_DEADZONE || joyButtonPressed) {
        Serial.print(F("SELECTING ITEM "));
        Serial.println(menuSelection);
        joyButtonPressed = false; // Consume press
        currentState = STATE_MAIN; // Go back for testing
        delay(200);
      }

      // Back to main
      if (joyX < -JOY_DEADZONE) {
        Serial.println(F("-> BACK TO MAIN"));
        currentState = STATE_MAIN;
        delay(200);
      }
      break;
  }

  display.display();
  delay(50); // Small delay for stability
}