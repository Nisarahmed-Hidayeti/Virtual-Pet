/*
  Arduino Nano Tamagotchi-Style Virtual Pet
  Based on the deliverables and specifications.
*/

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <EEPROM.h>

// Pin definitions
#define OLED_SDA A4
#define OLED_SCL A5
#define JOY_X_PIN A0
#define JOY_Y_PIN A1
#define JOY_BUTTON_PIN 3
#define BUZZER_PIN 9

// OLED settings
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define OLED_ADDRESS 0x3C // Default address, will try 0x3D if fails

// Joystick deadzone
#define JOY_DEADZONE 80

// EEPROM save version
#define SAVE_VERSION 1

// EEPROM address where we store our data
#define EEPROM_START_ADDR 0

// State machine
typedef enum {
  STATE_STARTUP,
  STATE_MAIN,
  STATE_MENU,
  STATE_FEEDING,
  STATE_PLAYING,
  STATE_CLEANING,
  STATE_SLEEPING,
  STATE_STATUS,
  STATE_GAMEOVER
} State;

// Pet stats structure
struct PetStats {
  uint8_t hunger;      // 0-100
  uint8_t happiness;   // 0-100
  uint8_t energy;      // 0-100
  uint8_t health;      // 0-100
  uint8_t cleanliness; // 0-100
  uint16_t age;        // in days or ticks
  uint16_t experience; // total XP
  uint8_t level;       // 1-99
  uint8_t saveVersion; // for EEPROM validation
};

// Global objects
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
PetStats pet;
State currentState = STATE_STARTUP;
unsigned long lastUpdate = 0;
unsigned long lastSave = 0;
const unsigned long UPDATE_INTERVAL_MS = 5000;  // Update stats every 5 seconds
const unsigned long SAVE_INTERVAL_MS = 30000;   // Save to EEPROM every 30 seconds
const unsigned long AGE_INTERVAL_MS = 10000;    // Age increases every 10 seconds

// Menu items
const char* menuItems[] = {
  "FOOD",
  "PLAY",
  "CLEAN",
  "SLEEP",
  "CARE",
  "STATUS"
};
const uint8_t menuItemCount = sizeof(menuItems) / sizeof(menuItems[0]);
uint8_t menuSelection = 0;

// Input variables
int joyX = 0;
int joyY = 0;
bool joyButtonPressed = false;
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 50;

// Forward declarations
void initOLED();
void drawStartupScreen();
void drawMainScreen();
void drawMenu();
void drawFeeding();
void drawPlaying();
void drawCleaning();
void drawSleeping();
void drawStatus();
void drawGameOver();
void updatePet();
void savePet();
void loadPet();
void handleInput();
void playSFX(int frequency, int duration);
void resetPet();

// Bitmap arrays for pet expressions (simple 16x16 placeholders - to be replaced with actual art)
const unsigned char petIdle[16] PROGMEM = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};
const unsigned char petHappy[16] PROGMEM = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};
// ... other expressions omitted for brevity, but we'll implement real ones later

void setup() {
  Serial.begin(115200);
  pinMode(JOY_BUTTON_PIN, INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT);

  // Initialize OLED
  initOLED();

  // Load or initialize pet
  loadPet();

  // Show startup screen
  currentState = STATE_STARTUP;
  lastUpdate = millis();
  drawStartupScreen();
}

void loop() {
  unsigned long now = millis();

  // Handle input
  handleInput();

  // Update pet stats periodically
  if (now - lastUpdate >= UPDATE_INTERVAL_MS) {
    lastUpdate = now;
    updatePet();
  }

  // Save to EEPROM periodically
  if (now - lastSave >= SAVE_INTERVAL_MS) {
    lastSave = now;
    savePet();
  }

  // Draw current state
  switch (currentState) {
    case STATE_STARTUP:
      // Startup screen is shown for a fixed time then transitions
      if (now - lastUpdate > 2000) { // 2 seconds startup
        currentState = STATE_MAIN;
        drawMainScreen();
      }
      break;
    case STATE_MAIN:
      drawMainScreen();
      break;
    case STATE_MENU:
      drawMenu();
      break;
    case STATE_FEEDING:
      drawFeeding();
      break;
    case STATE_PLAYING:
      drawPlaying();
      break;
    case STATE_CLEANING:
      drawCleaning();
      break;
    case STATE_SLEEPING:
      drawSleeping();
      break;
    case STATE_STATUS:
      drawStatus();
      break;
    case STATE_GAMEOVER:
      drawGameOver();
      break;
  }

  // Small delay to prevent excessive looping
  delay(10);
}

void initOLED() {
  Wire.begin(); // Uses default SDA/SCL pins (A4/A5 on Nano)

  // Try address 0x3C
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) {
    // If fails, try 0x3D
    uint8_t altAddress = 0x3D;
    if (!display.begin(SSD1306_SWITCHCAPVCC, altAddress)) {
      // If both fail, we'll keep trying but show error
      Serial.println("OLED initialization failed for both addresses");
      // We'll continue anyway; maybe it'll work later
    } else {
      Serial.println("OLED initialized at address 0x3D");
    }
  } else {
    Serial.println("OLED initialized at address 0x3C");
  }

  display.clearDisplay();
  display.display();
}

void drawStartupScreen() {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(10, 10);
  display.println(F("MY PET"));
  display.setCursor(10, 30);
  display.println(F("READY!"));
  display.display();
}

void drawMainScreen() {
  display.clearDisplay();

  // Draw status bar (top 16 pixels - yellow section conceptually)
  display.fillRect(0, 0, SCREEN_WIDTH, 16, SSD1306_WHITE);
  display.setTextColor(SSD1306_BLACK, SSD1306_WHITE); // Inverted text for status bar
  display.setCursor(2, 2);
  display.print(F("HP:"));
  display.print(pet.health);
  display.setCursor(30, 2);
  display.print(F("Hun:"));
  display.print(pet.hunger);

  // Draw pet in the middle (blue section)
  // We'll draw a simple placeholder for now
  display.fillRect(SCREEN_WIDTH/2 - 8, SCREEN_HEIGHT/2 - 8, 16, 16, SSD1306_WHITE);

  // Draw level
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(2, 20);
  display.print(F("Lv:"));
  display.print(pet.level);

  display.display();
}

void drawMenu() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  // Draw menu items
  for (uint8_t i = 0; i < menuItemCount; i++) {
    if (i == menuSelection) {
      display.fillRect(0, i*10, SCREEN_WIDTH, 10, SSD1306_WHITE);
      display.setTextColor(SSD1306_BLACK);
    } else {
      display.setTextColor(SSD1306_WHITE);
    }
    display.setCursor(0, i*10);
    display.print(menuItems[i]);
  }

  display.display();
}

void drawFeeding() {
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(10, 20);
  display.print(F("FEEDING..."));
  display.display();

  // Apply feeding effect
  pet.hunger = min(100, pet.hunger + 25);
  pet.happiness = min(100, pet.happiness + 10);
  pet.experience += 5;
  playSFX(1000, 100);

  // Return to main after short delay
  static unsigned long feedingStart = 0;
  if (feedingStart == 0) feedingStart = millis();
  if (millis() - feedingStart > 1000) {
    feedingStart = 0;
    currentState = STATE_MAIN;
    drawMainScreen();
  }
}

void drawPlaying() {
  // Simple catch game placeholder
  display.clearDisplay();
  display.setCursor(10, 20);
  display.print(F("PLAYING..."));
  display.display();
  // TODO: Implement actual catch game
  // For now, just return after a short time
  static unsigned long playStart = 0;
  if (playStart == 0) playStart = millis();
  if (millis() - playStart > 2000) {
    playStart = 0;
    currentState = STATE_MAIN;
    drawMainScreen();
  }
}

void drawCleaning() {
  display.clearDisplay();
  display.setCursor(10, 20);
  display.print(F("CLEANING..."));
  display.display();
  pet.cleanliness = min(100, pet.cleanliness + 20);
  pet.happiness = min(100, pet.happiness + 5);
  pet.experience += 3;
  playSFX(500, 100);

  static unsigned long cleanStart = 0;
  if (cleanStart == 0) cleanStart = millis();
  if (millis() - cleanStart > 1000) {
    cleanStart = 0;
    currentState = STATE_MAIN;
    drawMainScreen();
  }
}

void drawSleeping() {
  display.clearDisplay();
  display.setCursor(10, 20);
  display.print(F("SLEEPING..."));
  display.display();
  // Gradually increase energy while sleeping
  if (pet.energy < 100) pet.energy++;
  playSFX(200, 50); // Soft snore sound

  // Wake on button press
  if (joyButtonPressed) {
    joyButtonPressed = false;
    currentState = STATE_MAIN;
    drawMainScreen();
    playSFX(800, 100);
  }
}

void drawStatus() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print(F("HP: ")); display.print(pet.health);
  display.setCursor(0, 10);
  display.print(F("Hunger: ")); display.print(pet.hunger);
  display.setCursor(0, 20);
  display.print(F("Happy: ")); display.print(pet.happiness);
  display.setCursor(0, 30);
  display.print(F("Energy: ")); display.print(pet.energy);
  display.setCursor(0, 40);
  display.print(F("Clean: ")); display.print(pet.cleanliness);
  display.setCursor(0, 50);
  display.print(F("Age: ")); display.print(pet.age);
  display.setCursor(70, 0);
  display.print(F("Lv: ")); display.print(pet.level);
  display.setCursor(70, 10);
  display.print(F("XP: ")); display.print(pet.experience);
  display.display();

  // Exit on button press or left
  if (joyButtonPressed) {
    joyButtonPressed = false;
    currentState = STATE_MAIN;
    drawMainScreen();
  }
}

void drawGameOver() {
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(10, 10);
  display.print(F("GAME OVER"));
  display.setCursor(10, 30);
  display.print(F("Press to"));
  display.setCursor(10, 40);
  display.print(F("restart"));
  display.display();

  if (joyButtonPressed) {
    joyButtonPressed = false;
    resetPet();
    currentState = STATE_MAIN;
    drawMainScreen();
  }
}

void updatePet() {
  // Gradual stat decay
  if (pet.hunger > 0) pet.hunger -= 1;
  if (pet.happiness > 0) pet.happiness -= 1;
  if (pet.cleanliness > 0) pet.cleanliness -= 1;
  if (! (currentState == STATE_SLEEPING) && pet.energy > 0) pet.energy -= 1;

  // Increase age periodically
  static unsigned long lastAgeUpdate = 0;
  if (millis() - lastAgeUpdate >= AGE_INTERVAL_MS) {
    lastAgeUpdate = millis();
    pet.age++;
    pet.experience += 1; // XP for aging

    // Check for level up
    uint16_t xpForNextLevel = 15 * pet.level * 2; // Base 15 * level * factor 2
    if (pet.experience >= xpForNextLevel) {
      pet.level++;
      pet.experience -= xpForNextLevel;
      playSFX(1500, 200); // Level up sound
    }
  }

  // Health decreases if needs are low
  if (pet.hunger < 20 || pet.happiness < 20 || pet.cleanliness < 20) {
    if (pet.health > 0) pet.health -= 1;
  }

  // Check for game over
  if (pet.health <= 0) {
    currentState = STATE_GAMEOVER;
    drawGameOver();
  }
}

void savePet() {
  // Only save if we have valid data
  if (EEPROM.length() < EEPROM_START_ADDR + sizeof(PetStats)) return;

  pet.saveVersion = SAVE_VERSION;
  EEPROM.put(EEPROM_START_ADDR, pet);
}

void loadPet() {
  if (EEPROM.length() < EEPROM_START_ADDR + sizeof(PetStats)) {
    // EEPROM too small, initialize new pet
    resetPet();
    return;
  }

  PetStats tempPet;
  EEPROM.get(EEPROM_START_ADDR, tempPet);

  // Validate version and data
  if (tempPet.saveVersion == SAVE_VERSION &&
      tempPet.health <= 100 && tempPet.hunger <= 100 &&
      tempPet.happiness <= 100 && pet.energy <= 100 &&
      pet.cleanliness <= 100) {
    pet = tempPet;
  } else {
    // Invalid data, initialize new pet
    resetPet();
  }
}

void handleInput() {
  // Read joystick
  joyX = analogRead(JOY_X_PIN) - 512; // Center around 0
  joyY = analogRead(JOY_Y_PIN) - 512;

  // Read button with debounce
  bool buttonReading = !digitalRead(JOY_BUTTON_PIN); // Active low

  if (buttonReading && (millis() - lastDebounceTime > debounceDelay)) {
    joyButtonPressed = true;
    lastDebounceTime = millis();
  } else if (!buttonReading) {
    joyButtonPressed = false;
  }

  // Handle directional input based on state
  switch (currentState) {
    case STATE_MAIN:
      if (joyY < -JOY_DEADZONE) {
        // Up - maybe show hint
      } else if (joyY > JOY_DEADZONE) {
        // Down - maybe show hint
      } else if (joyX < -JOY_DEADZONE) {
        // Left - go to status
        currentState = STATE_STATUS;
        drawStatus();
      } else if (joyX > JOY_DEADZONE) {
        // Right - open menu
        currentState = STATE_MENU;
        drawMenu();
      }
      break;

    case STATE_MENU:
      if (joyY < -JOY_DEADZONE) {
        menuSelection = (menuSelection + menuItemCount - 1) % menuItemCount;
        delay(150); // Simple debounce for navigation
      } else if (joyY > JOY_DEADZONE) {
        menuSelection = (menuSelection + 1) % menuItemCount;
        delay(150);
      } else if (joyX > JOY_DEADZONE || joyButtonPressed) {
        // Select menu item
        joyButtonPressed = false; // Consume the press
        switch (menuSelection) {
          case 0: currentState = STATE_FEEDING; break;
          case 1: currentState = STATE_PLAYING; break;
          case 2: currentState = STATE_CLEANING; break;
          case 3: currentState = STATE_SLEEPING; break;
          case 4: currentState = STATE_STATUS; break; // CARE -> STATUS for now
          case 5: currentState = STATE_STATUS; break;
        }
        if (currentState != STATE_MENU) {
          // Reset feeding/playing etc timers if needed
          switch (currentState) {
            case STATE_FEEDING: /* feedingStart = millis(); */ break;
            case STATE_PLAYING: /* playStart = millis(); */ break;
            case STATE_CLEANING: /* cleanStart = millis(); */ break;
          }
          // Draw the new state
          switch (currentState) {
            case STATE_FEEDING: drawFeeding(); break;
            case STATE_PLAYING: drawPlaying(); break;
            case STATE_CLEANING: drawCleaning(); break;
            case STATE_SLEEPING: drawSleeping(); break;
            case STATE_STATUS: drawStatus(); break;
          }
        }
        delay(200); // Prevent multiple triggers
      } else if (joyX < -JOY_DEADZONE) {
        // Back to main
        currentState = STATE_MAIN;
        drawMainScreen();
        delay(200);
      }
      break;

    // Other states handle their own exits via timers or button presses
    default:
      // In substates, button usually exits
      if (joyButtonPressed &&
          (currentState == STATE_FEEDING ||
           currentState == STATE_PLAYING ||
           currentState == STATE_CLEANING ||
           currentState == STATE_SLEEPING)) {
        joyButtonPressed = false;
        currentState = STATE_MAIN;
        drawMainScreen();
        delay(200);
      }
      break;
  }
}

void playSFX(int frequency, int duration) {
  tone(BUZZER_PIN, frequency, duration);
  // Note: tone() is non-blocking on AVR for short durations
}

void resetPet() {
  pet.health = 100;
  pet.hunger = 50;
  pet.happiness = 50;
  pet.energy = 50;
  pet.cleanliness = 50;
  pet.age = 0;
  pet.experience = 0;
  pet.level = 1;
  pet.saveVersion = SAVE_VERSION;
  savePet(); // Save initial state
}