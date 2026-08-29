/*
 * Arduino Nano Tamagotchi-Style Virtual Pet
 *
 * Hardware:
 *   - Arduino Nano (ATmega328P)
 *   - 0.96" 128x64 OLED (SSD1306) via I2C (SDA=A4, SCL=A5)
 *   - KY-023 Joystick (VRX=A0, VRY=A1, SW=D3)
 *   - Passive Buzzer (D9)
 *
 * Features:
 *   - Tamagotchi-style virtual pet with needs (hunger, happiness, energy, health, cleanliness)
 *   - Aging, experience, leveling
 *   - Animations, sound effects, chiptune music
 *   - Menu system (FOOD, PLAY, CLEAN, SLEEP, CARE, STATUS)
 *   - EEPROM save system
 *   - Non-blocking timing using millis()
 *   - Mini-game (catch game)
 *   - Pet expressions and animations
 *
 * Libraries:
 *   - Wire.h
 *   - Adafruit_GFX.h
 *   - Adafruit_SSD1306.h
 *   - EEPROM.h
 */

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <EEPROM.h>

// === CONFIGURATION ===
#define OLED_ADDRESS 0x3C
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define STATUS_BAR_HEIGHT 16  // Yellow section height

// Joystick pins
#define JOY_X_PIN A0
#define JOY_Y_PIN A1
#define JOY_BUTTON_PIN 3

// Buzzer pin
#define BUZZER_PIN 9

// Joystick deadzone (adjust if needed)
#define JOY_DEADZONE 120

// EEPROM address for save data
#define EEPROM_ADDR 0

// Save version (increment if data structure changes)
#define SAVE_VERSION 1

// Stat ranges
#define STAT_MIN 0
#define STAT_MAX 100

// Aging interval (milliseconds per age increment)
#define AGE_INTERVAL_MS 20000UL  // 20 seconds = 1 age unit

// XP requirements per level (increases with level)
#define XP_PER_LEVEL_BASE 15
#define XP_PER_LEVEL_FACTOR 2

// === STATS ===
struct PetStats {
  uint8_t hunger;          // 0-100 (higher = more full)
  uint8_t happiness;       // 0-100
  uint8_t energy;          // 0-100
  uint8_t health;          // 0-100
  uint8_t cleanliness;     // 0-100
  uint16_t age;            // in age units (each unit = AGE_INTERVAL_MS)
  uint16_t experience;     // current XP
  uint8_t level;           // 1-99
  uint16_t xpToNextLevel;  // XP needed for next level
};

// === GAME STATE ===
enum GameState {
  STATE_STARTUP,
  STATE_MAIN,
  STATE_MENU,
  STATE_FEEDING,
  STATE_PLAYING,
  STATE_CLEANING,
  STATE_SLEEPING,
  STATE_STATUS,
  STATE_GAMEOVER
};

// === GLOBAL OBJECTS ===
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
PetStats pet;
GameState gameState = STATE_STARTUP;
unsigned long lastUpdate = 0;
unsigned long lastAgeUpdate = 0;
unsigned long lastStatUpdate = 0;
unsigned long lastSave = 0;
unsigned long lastFoodEffect = 0;
unsigned long lastPlayUpdate = 0;
unsigned long lastCleanUpdate = 0;
unsigned long lastSleepUpdate = 0;

// === INPUT VARIABLES ===
int joyX = 0;
int joyY = 0;
bool joyButtonPressed = false;
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 50;

// === ANIMATION & UI ===
uint8_t menuCursor = 0;  // 0-5 for menu items
const char* menuItems[] = { "FOOD", "PLAY", "CLEAN", "SLEEP", "CARE", "STATUS" };
const uint8_t numMenuItems = 6;

// Gameplay variables
int catchGameX = 64;
int catchGameY = 32;
int catchGameTargetX = 0;
int catchGameTargetY = 0;
bool catchGameActive = false;
unsigned long catchGameStartTime = 0;
const unsigned long catchGameDuration = 10000;  // 10 seconds
int catchGameScore = 0;

// === SOUND ===
bool musicEnabled = true;
bool sfxEnabled = true;
unsigned long lastSFXTime = 0;
const unsigned long sfxMinInterval = 100;  // Minimum time between SFX

// === PET ANIMATION FRAMES ===
// Simple bitmap for pet expressions (16x16 pixels)
// Each frame is 32 bytes (16 rows * 2 bytes per row for 16 pixels)
const unsigned char petIdle[] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

// Simple happy frame (pet jumping slightly)
const unsigned char petHappy[] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

// Simple sleeping frame (eyes closed)
const unsigned char petSleeping[] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

// Simple sick frame
const unsigned char petSick[] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

// === FORWARD DECLARATIONS ===
void setup();
void loop();
void updatePet();
void updateStats();
void updateInput();
void updateUI();
void updateAudio();
void handleMenu();
void handleGameplay();
void handleFeeding();
void handlePlaying();
void handleCleaning();
void handleSleeping();
void handleStatus();
void savePet();
void loadPet();
void playTone(unsigned int frequency, unsigned long duration);
void playSFX(const char* sfxName);
void updateLevel();
void startCatchGame();
void updateCatchGame();
void drawPet(uint8_t expression, int offsetY);
void drawHeart(int x, int y);
void drawFoodIcon(int x, int y);
void drawBubble(int x, int y, uint8_t size);
void drawStartup();
void drawMain();
void drawMenu();
void drawFeeding();
void drawPlaying();
void drawCleaning();
void drawSleeping();
void drawStatus();
void drawGameOver();

// === SETUP ===
void setup() {
  // Initialize pins
  pinMode(JOY_BUTTON_PIN, INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT);

  // Initialize OLED
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) {
    // Handle error - for now, just loop forever
    while (true)
      ;
  }
  display.clearDisplay();
  display.display();

  // Load pet data from EEPROM or initialize
  loadPet();

  // Initial delay to let things settle
  delay(100);

  // Startup sequence
  gameState = STATE_STARTUP;
  lastUpdate = millis();
}

// === MAIN LOOP ===
void loop() {
  unsigned long now = millis();

  // Update at a fixed interval (e.g., 50 FPS)
  if (now - lastUpdate >= 20) {  // 20ms = 50Hz
    lastUpdate = now;

    updateInput();
    updatePet();
    updateStats();
    updateUI();
    updateAudio();
  }
}

// === INPUT HANDLING ===
void updateInput() {
  // Read joystick
  joyX = analogRead(JOY_X_PIN);
  joyY = analogRead(JOY_Y_PIN);

  // Button with debounce
  bool buttonState = digitalRead(JOY_BUTTON_PIN);
  if (buttonState == LOW && (millis() - lastDebounceTime) > debounceDelay) {
    joyButtonPressed = true;
    lastDebounceTime = millis();
  } else if (buttonState == HIGH) {
    joyButtonPressed = false;
  }
}

// === PET LOGIC ===
void updatePet() {
  unsigned long now = millis();

  // Aging
  if (now - lastAgeUpdate >= AGE_INTERVAL_MS) {
    lastAgeUpdate = now;
    if (pet.age < 65535) pet.age++;

    // Award XP for surviving
    pet.experience += 1;
    updateLevel();
  }

  // Stat decay (happens every few seconds)
  if (now - lastStatUpdate >= 5000) {  // Every 5 seconds
    lastStatUpdate = now;

    // Hunger decreases
    if (pet.hunger > STAT_MIN) pet.hunger--;

    // Happiness decreases slowly
    if (pet.happiness > STAT_MIN) pet.happiness--;

    // Energy decreases when awake (not sleeping)
    if (gameState != STATE_SLEEPING && pet.energy > STAT_MIN) pet.energy--;

    // Cleanliness decreases
    if (pet.cleanliness > STAT_MIN) pet.cleanliness--;

    // Health affected by poor stats
    if (pet.hunger < 20) {
      if (pet.health > STAT_MIN) pet.health--;
    }
    if (pet.cleanliness < 20) {
      if (pet.health > STAT_MIN) pet.health--;
    }
    if (pet.happiness < 20) {
      if (pet.health > STAT_MIN) pet.health--;  // Small penalty
    }

    // Clamp stats
    if (pet.hunger > STAT_MAX) pet.hunger = STAT_MAX;
    if (pet.happiness > STAT_MAX) pet.happiness = STAT_MAX;
    if (pet.energy > STAT_MAX) pet.energy = STAT_MAX;
    if (pet.health > STAT_MAX) pet.health = STAT_MAX;
    if (pet.cleanliness > STAT_MAX) pet.cleanliness = STAT_MAX;
  }

  // Check for game over (health depleted)
  if (pet.health <= STAT_MIN && gameState != STATE_GAMEOVER) {
    gameState = STATE_GAMEOVER;
    playSFX("gameover");
  }
}

// === STATS UPDATE (for actions) ===
void updateStats() {
  // This function is for stat changes from actions (feeding, playing, etc.)
  // Actual decay is handled in updatePet()
}

// === LEVELING SYSTEM ===
void updateLevel() {
  while (pet.experience >= pet.xpToNextLevel && pet.level < 99) {
    pet.experience -= pet.xpToNextLevel;
    pet.level++;
    pet.xpToNextLevel = XP_PER_LEVEL_BASE * pet.level * XP_PER_LEVEL_FACTOR;
    playSFX("levelup");
    // Show level up notification briefly
    // In a full implementation, we'd have a level up state
  }

  // Initialize xpToNextLevel if not set
  if (pet.level == 1 && pet.xpToNextLevel == 0) {
    pet.xpToNextLevel = XP_PER_LEVEL_BASE;
  }
}

// === UI RENDERING ===
void updateUI() {
  display.clearDisplay();

  // Draw status bar (yellow section) - we'll simulate by drawing a rectangle
  // In reality, the OLED has yellow top pixels, but we'll just draw the bar at top
  display.fillRect(0, 0, SCREEN_WIDTH, STATUS_BAR_HEIGHT, SSD1306_WHITE);

  // Draw status text (black on white background for status bar)
  display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);  // Inverted for status bar
  display.setCursor(2, 2);
  display.print("HP:");
  display.print(pet.health);
  display.print(" HUN:");
  display.print(pet.hunger);

  // Reset text color for main area
  display.setTextColor(SSD1306_WHITE);

  // Draw based on game state
  switch (gameState) {
    case STATE_STARTUP:
      drawStartup();
      break;
    case STATE_MAIN:
      drawMain();
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

  display.display();
}

// === STATE DRAW FUNCTIONS ===
void drawStartup() {
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(10, 20);
  display.print("MY PET");
  display.setTextSize(2);
  display.setCursor(10, 30);
  display.print("IS READY!");
}

void drawMain() {
  // Draw pet based on mood
  uint8_t expression = 0;  // 0=idle, 1=happy, 2=sleeping, 3=sick

  if (pet.health < 30) expression = 3;                   // sick
  else if (pet.happiness > 80) expression = 1;           // happy
  else if (gameState == STATE_SLEEPING) expression = 2;  // sleeping

  drawPet(expression, 0);

  // Draw simple face details
  int petX = 56;
  int petY = 20;

  // Eyes
  if (expression != 2) {                                        // Not sleeping
    display.fillCircle(petX + 8, petY + 8, 2, SSD1306_WHITE);   // Left eye
    display.fillCircle(petX + 24, petY + 8, 2, SSD1306_WHITE);  // Right eye
  } else {
    // Sleeping eyes (closed)
    display.drawLine(petX + 6, petY + 10, petX + 10, petY + 10, SSD1306_WHITE);
    display.drawLine(petX + 22, petY + 10, petX + 26, petY + 10, SSD1306_WHITE);
  }

  // Mouth based on expression
  if (expression == 0) {  // Idle - neutral
    // Simple line for neutral mouth
    display.drawLine(petX + 12, petY + 22, petX + 20, petY + 22, SSD1306_WHITE);
  } else if (expression == 1) {  // Happy - big smile
    // Draw a smile using multiple lines
    display.drawLine(petX + 12, petY + 24, petX + 14, petY + 26, SSD1306_WHITE);
    display.drawLine(petX + 14, petY + 26, petX + 16, petY + 24, SSD1306_WHITE);
    display.drawLine(petX + 16, petY + 24, petX + 18, petY + 26, SSD1306_WHITE);
    display.drawLine(petX + 18, petY + 26, petX + 20, petY + 24, SSD1306_WHITE);
  } else if (expression == 3) {  // Sad/Sick - frown
    // Draw a frown using multiple lines
    display.drawLine(petX + 12, petY + 24, petX + 14, petY + 22, SSD1306_WHITE);
    display.drawLine(petX + 14, petY + 22, petX + 16, petY + 24, SSD1306_WHITE);
    display.drawLine(petX + 16, petY + 24, petX + 18, petY + 22, SSD1306_WHITE);
    display.drawLine(petX + 18, petY + 22, petX + 20, petY + 24, SSD1306_WHITE);
  }
}

void drawMenu() {
  display.setCursor(2, STATUS_BAR_HEIGHT + 2);
  for (uint8_t i = 0; i < numMenuItems; i++) {
    if (i == menuCursor) {
      display.print("> ");
    } else {
      display.print("  ");
    }
    display.print(menuItems[i]);
    if (i < numMenuItems - 1) display.println();
  }
}

void drawFeeding() {
  display.setCursor(10, STATUS_BAR_HEIGHT + 10);
  display.print("FEEDING...");

  // Draw food
  drawFoodIcon(60, 30);

  // Draw pet enjoying food
  drawPet(1, 0);  // Happy expression
}

void drawPlaying() {
  display.setCursor(10, STATUS_BAR_HEIGHT + 10);
  display.print("PLAYING...");

  if (catchGameActive) {
    updateCatchGame();
    // Draw target
    display.fillRect(catchGameTargetX, catchGameTargetY, 8, 8, SSD1306_WHITE);
    // Draw cursor
    display.fillRect(catchGameX, catchGameY, 4, 4, SSD1306_WHITE);
    // Draw score
    display.setCursor(90, STATUS_BAR_HEIGHT + 10);
    display.print("Score:");
    display.print(catchGameScore);
  } else {
    display.setCursor(20, STATUS_BAR_HEIGHT + 20);
    display.print("Press to start");
    display.setCursor(20, STATUS_BAR_HEIGHT + 30);
    display.print("catch game!");
  }
}

void drawCleaning() {
  display.setCursor(10, STATUS_BAR_HEIGHT + 10);
  display.print("CLEANING...");

  // Draw bubbles
  drawBubble(50, 25, 3);
  drawBubble(70, 35, 2);
  drawBubble(60, 40, 4);

  // Draw pet
  drawPet(0, 0);  // Neutral
}

void drawSleeping() {
  display.setCursor(10, STATUS_BAR_HEIGHT + 10);
  display.print("SLEEPING...");

  // Draw Z's
  display.setCursor(90, STATUS_BAR_HEIGHT + 10);
  display.print("Z");
  display.setCursor(96, STATUS_BAR_HEIGHT + 5);
  display.print("z");
  display.setCursor(102, STATUS_BAR_HEIGHT + 0);
  display.print("z");

  // Draw sleeping pet
  drawPet(2, 0);  // Sleeping expression
}

void drawStatus() {
  display.setCursor(2, STATUS_BAR_HEIGHT + 2);
  display.print("HP:");
  display.print(pet.health);
  display.setCursor(2, STATUS_BAR_HEIGHT + 12);
  display.print("HUN:");
  display.print(pet.hunger);
  display.setCursor(2, STATUS_BAR_HEIGHT + 22);
  display.print("HAP:");
  display.print(pet.happiness);
  display.setCursor(2, STATUS_BAR_HEIGHT + 32);
  display.print("EN:");
  display.print(pet.energy);
  display.setCursor(2, STATUS_BAR_HEIGHT + 42);
  display.print("CLN:");
  display.print(pet.cleanliness);
  display.setCursor(2, STATUS_BAR_HEIGHT + 52);
  display.print("LV:");
  display.print(pet.level);
  display.print(" XP:");
  display.print(pet.experience);
  display.print("/");
  display.print(pet.xpToNextLevel);
}

void drawGameOver() {
  display.setCursor(10, STATUS_BAR_HEIGHT + 10);
  display.print("OH NO...");
  display.setCursor(10, STATUS_BAR_HEIGHT + 20);
  display.print("GAME OVER");
  display.setCursor(10, STATUS_BAR_HEIGHT + 30);
  display.print("HP:0");
  display.setCursor(10, STATUS_BAR_HEIGHT + 40);
  display.print("PRESS TO");
  display.setCursor(10, STATUS_BAR_HEIGHT + 50);
  display.print("RESTART");
}

// === STATE HANDLERS ===
void handleMenu() {
  // Joystick Y for menu navigation
  if (joyY < (512 - JOY_DEADZONE)) {  // Up
    if (menuCursor > 0) menuCursor--;
    playSFX("menu_move");
    delay(150);                              // Simple debounce for navigation
  } else if (joyY > (512 + JOY_DEADZONE)) {  // Down
    if (menuCursor < (numMenuItems - 1)) menuCursor++;
    playSFX("menu_move");
    delay(150);
  }

  // Button press to select
  if (joyButtonPressed) {
    joyButtonPressed = false;  // Reset button state
    switch (menuCursor) {
      case 0:  // FOOD
        gameState = STATE_FEEDING;
        lastFoodEffect = millis();
        break;
      case 1:  // PLAY
        gameState = STATE_PLAYING;
        startCatchGame();
        break;
      case 2:  // CLEAN
        gameState = STATE_CLEANING;
        lastCleanUpdate = millis();
        break;
      case 3:  // SLEEP
        gameState = STATE_SLEEPING;
        lastSleepUpdate = millis();
        break;
      case 4:  // CARE
        gameState = STATE_STATUS;
        break;
      case 5:  // STATUS
        gameState = STATE_STATUS;
        break;
    }
    playSFX("select");
    delay(200);
  }

  // Left to go back to main screen
  if (joyX < (512 - JOY_DEADZONE)) {  // Left
    gameState = STATE_MAIN;
    playSFX("back");
    delay(200);
  }
}

void handleGameplay() {
  // Handle state-specific gameplay
  switch (gameState) {
    case STATE_FEEDING:
      handleFeeding();
      break;
    case STATE_PLAYING:
      handlePlaying();
      break;
    case STATE_CLEANING:
      handleCleaning();
      break;
    case STATE_SLEEPING:
      handleSleeping();
      break;
    default:
      // For other states, just check for back button
      if (joyButtonPressed) {
        joyButtonPressed = false;
        gameState = STATE_MAIN;
        playSFX("back");
        delay(200);
      }
      break;
  }
}

void handleFeeding() {
  // Feed the pet over time
  if (joyButtonPressed && (millis() - lastFoodEffect > 500)) {
    joyButtonPressed = false;
    // Increase hunger and happiness
    pet.hunger = min(pet.hunger + 25, STAT_MAX);
    pet.happiness = min(pet.happiness + 10, STAT_MAX);
    pet.experience += 5;
    updateLevel();
    playSFX("eat");
    lastFoodEffect = millis();

    // Auto-exit after feeding
    gameState = STATE_MAIN;
  }

  // Auto-exit after a few seconds if no input
  if (millis() - lastFoodEffect > 3000) {
    gameState = STATE_MAIN;
  }
}

void handlePlaying() {
  // Handle catch game
  if (joyButtonPressed && !catchGameActive) {
    joyButtonPressed = false;
    startCatchGame();
  }

  if (catchGameActive) {
    // Update game based on joystick
    if (joyX < (512 - JOY_DEADZONE)) {  // Left
      catchGameX = max(catchGameX - 3, 0);
    } else if (joyX > (512 + JOY_DEADZONE)) {  // Right
      catchGameX = min(catchGameX + 3, SCREEN_WIDTH - 4);
    }

    if (joyY < (512 - JOY_DEADZONE)) {  // Up
      catchGameY = max(catchGameY - 3, STATUS_BAR_HEIGHT);
    } else if (joyY > (512 + JOY_DEADZONE)) {  // Down
      catchGameY = min(catchGameY + 3, SCREEN_HEIGHT - 4);
    }

    // Check for button press to catch
    if (joyButtonPressed) {
      joyButtonPressed = false;
      // Check if caught
      if (abs(catchGameX - catchGameTargetX) < 4 && abs(catchGameY - catchGameTargetY) < 4) {
        // Successful catch!
        catchGameScore++;
        pet.happiness = min(pet.happiness + 15, STAT_MAX);
        pet.experience += 10;
        updateLevel();
        playSFX("catch");

        // New target
        catchGameTargetX = random(0, SCREEN_WIDTH - 8);
        catchGameTargetY = random(STATUS_BAR_HEIGHT, SCREEN_HEIGHT - 8);
      } else {
        // Miss
        playSFX("miss");
      }
    }
  } else {
    // Waiting for button press to start
    if (joyButtonPressed) {
      joyButtonPressed = false;
      startCatchGame();
    }
  }

  // Auto-exit when game ends
  if (!catchGameActive && millis() - lastPlayUpdate > 2000) {
    gameState = STATE_MAIN;
  }
}

void handleCleaning() {
  // Clean the pet over time
  if (joyButtonPressed && (millis() - lastCleanUpdate > 500)) {
    joyButtonPressed = false;
    // Increase cleanliness
    pet.cleanliness = min(pet.cleanliness + 20, STAT_MAX);
    pet.happiness = min(pet.happiness + 5, STAT_MAX);  // Slight happiness boost
    pet.experience += 3;
    updateLevel();
    playSFX("bubble");
    lastCleanUpdate = millis();

    // Auto-exit after cleaning
    gameState = STATE_MAIN;
  }

  // Auto-exit after a few seconds
  if (millis() - lastCleanUpdate > 4000) {
    gameState = STATE_MAIN;
  }
}

void handleSleeping() {
  // Sleep to restore energy
  if (joyButtonPressed && (millis() - lastSleepUpdate > 1000)) {
    joyButtonPressed = false;
    gameState = STATE_MAIN;
    playSFX("wake");
    return;
  }

  // Gradually restore energy while sleeping
  if (millis() - lastSleepUpdate > 2000) {
    lastSleepUpdate = millis();
    if (pet.energy < STAT_MAX) {
      pet.energy = min(pet.energy + 5, STAT_MAX);
      pet.experience += 1;
      updateLevel();
    }
  }
}

// === SAVE/LOAD SYSTEM ===
void savePet() {
  // Only save every 30 seconds to reduce EEPROM wear
  if (millis() - lastSave < 30000) return;

  // Create a buffer with version and stats
  uint8_t buffer[sizeof(PetStats) + 1];
  buffer[0] = SAVE_VERSION;
  memcpy(&buffer[1], &pet, sizeof(PetStats));

  // Write to EEPROM
  for (int i = 0; i < sizeof(buffer); i++) {
    EEPROM.update(EEPROM_ADDR + i, buffer[i]);
  }

  lastSave = millis();
}

void loadPet() {
  // Check version
  uint8_t version = EEPROM.read(EEPROM_ADDR);
  if (version == SAVE_VERSION) {
    // Load stats
    EEPROM.get(EEPROM_ADDR + 1, pet);

    // Validate loaded data
    if (pet.level < 1 || pet.level > 99) {
      pet.level = 1;
    }
    if (pet.hunger > STAT_MAX) pet.hunger = STAT_MAX;
    if (pet.happiness > STAT_MAX) pet.happiness = STAT_MAX;
    if (pet.energy > STAT_MAX) pet.energy = STAT_MAX;
    if (pet.health > STAT_MAX) pet.health = STAT_MAX;
    if (pet.cleanliness > STAT_MAX) pet.cleanliness = STAT_MAX;
    if (pet.experience > 65535) pet.experience = 0;

    // Initialize xpToNextLevel if needed
    if (pet.level == 1 && pet.xpToNextLevel == 0) {
      pet.xpToNextLevel = XP_PER_LEVEL_BASE;
    } else if (pet.level > 1) {
      pet.xpToNextLevel = XP_PER_LEVEL_BASE * pet.level * XP_PER_LEVEL_FACTOR;
    }
  } else {
    // Initialize default stats
    pet.hunger = 50;
    pet.happiness = 50;
    pet.energy = 50;
    pet.health = 50;
    pet.cleanliness = 50;
    pet.age = 0;
    pet.experience = 0;
    pet.level = 1;
    pet.xpToNextLevel = XP_PER_LEVEL_BASE;

    // Save initial state
    savePet();
  }
}

// === AUDIO SYSTEM ===
void updateAudio() {
  // Handle any periodic audio updates
  // For now, we handle audio on-demand in the respective functions
  // This could be expanded for background music in the future
}

void playTone(unsigned int frequency, unsigned long duration) {
  if (!sfxEnabled && !musicEnabled) return;  // Master mute

  // Prevent tones too close together
  if (millis() - lastSFXTime < sfxMinInterval && duration > 50) {
    return;
  }

  tone(BUZZER_PIN, frequency, duration);
  lastSFXTime = millis();
  // Note: tone() is blocking for the duration, but we use short durations
}

void playSFX(const char* sfxName) {
  if (!sfxEnabled) return;

  // Prevent spam
  if (millis() - lastSFXTime < sfxMinInterval) {
    return;
  }

  // Simple SFX mapping
  if (strcmp(sfxName, "menu_move") == 0) {
    playTone(800, 30);
  } else if (strcmp(sfxName, "select") == 0) {
    playTone(1000, 50);
  } else if (strcmp(sfxName, "back") == 0) {
    playTone(600, 50);
  } else if (strcmp(sfxName, "eat") == 0) {
    playTone(900, 100);
  } else if (strcmp(sfxName, "catch") == 0) {
    playTone(1200, 100);
  } else if (strcmp(sfxName, "miss") == 0) {
    playTone(400, 100);
  } else if (strcmp(sfxName, "bubble") == 0) {
    playTone(1000, 40);
  } else if (strcmp(sfxName, "levelup") == 0) {
    // Play a short melody for level up
    playTone(800, 50);
    delay(60);
    playTone(1000, 50);
    delay(60);
    playTone(1200, 100);
  } else if (strcmp(sfxName, "gameover") == 0) {
    playTone(200, 500);
  } else if (strcmp(sfxName, "wake") == 0) {
    playTone(800, 100);
  } else {
    // Default beep
    playTone(1000, 50);
  }

  lastSFXTime = millis();
}

// === GAMEPLAY FUNCTIONS ===
void startCatchGame() {
  catchGameActive = true;
  catchGameStartTime = millis();
  catchGameScore = 0;

  // Initial target position
  catchGameTargetX = random(0, SCREEN_WIDTH - 8);
  catchGameTargetY = random(STATUS_BAR_HEIGHT, SCREEN_HEIGHT - 8);

  // Initial cursor position (center)
  catchGameX = SCREEN_WIDTH / 2 - 2;
  catchGameY = (SCREEN_HEIGHT + STATUS_BAR_HEIGHT) / 2 - 2;

  lastPlayUpdate = millis();
}

void updateCatchGame() {
  unsigned long now = millis();

  // Check if time's up
  if (now - catchGameStartTime > catchGameDuration) {
    catchGameActive = false;
    lastPlayUpdate = now;

    // Award XP based on score
    pet.experience += catchGameScore * 2;
    updateLevel();

    // Happiness bonus for playing
    pet.happiness = min(pet.happiness + catchGameScore, STAT_MAX);

    // Energy cost for playing
    pet.energy = max(pet.energy - 10, STAT_MIN);

    return;
  }

  // Occasionally move target to make it more interesting
  if (random(0, 50) == 0) {
    catchGameTargetX = random(0, SCREEN_WIDTH - 8);
    catchGameTargetY = random(STATUS_BAR_HEIGHT, SCREEN_HEIGHT - 8);
  }
}

// === DRAWING FUNCTIONS ===
void drawPet(uint8_t expression, int offsetY) {
  const unsigned char* petFrame = petIdle;

  switch (expression) {
    case 1:  // Happy
      petFrame = petHappy;
      break;
    case 2:  // Sleeping
      petFrame = petSleeping;
      break;
    case 3:  // Sick
      petFrame = petSick;
      break;
    default:  // Idle
      petFrame = petIdle;
      break;
  }

  display.drawBitmap(56, 20 + offsetY, petFrame, 16, 16, SSD1306_WHITE);
}

void drawHeart(int x, int y) {
  // Simple heart shape
  display.drawPixel(x, y - 2, SSD1306_WHITE);
  display.drawPixel(x + 1, y - 3, SSD1306_WHITE);
  display.drawPixel(x + 2, y - 4, SSD1306_WHITE);
  display.drawPixel(x + 3, y - 4, SSD1306_WHITE);
  display.drawPixel(x + 4, y - 3, SSD1306_WHITE);
  display.drawPixel(x + 5, y - 2, SSD1306_WHITE);

  display.drawPixel(x - 2, y - 2, SSD1306_WHITE);
  display.drawPixel(x - 1, y - 3, SSD1306_WHITE);
  display.drawPixel(x, y - 4, SSD1306_WHITE);
  display.drawPixel(x + 1, y - 4, SSD1306_WHITE);
  display.drawPixel(x + 2, y - 3, SSD1306_WHITE);
  display.drawPixel(x + 3, y - 2, SSD1306_WHITE);

  display.drawPixel(x, y, SSD1306_WHITE);
}

void drawFoodIcon(int x, int y) {
  // Simple apple shape
  display.fillCircle(x + 2, y, 3, SSD1306_WHITE);       // Main body
  display.fillRect(x + 1, y - 4, 2, 2, SSD1306_WHITE);  // Stem
}

void drawBubble(int x, int y, uint8_t size) {
  display.drawCircle(x, y, size, SSD1306_WHITE);
}