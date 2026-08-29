# Arduino Nano Tamagotchi-Style Virtual Pet - Deliverables

## 1. Final Pin Mapping

| Component | Arduino Nano Pin | Description |
|-----------|------------------|-------------|
| OLED SDA | A4 | I2C Data |
| OLED SCL | A5 | I2C Clock |
| Joystick VRX | A0 | X-axis (horizontal) |
| Joystick VRY | A1 | Y-axis (vertical) |
| Joystick SW | D3 | Button (active low, uses internal pull-up) |
| Passive Buzzer | D9 | PWM/tone output |
| OLED VCC | 3.3V or 5V | Power (check your OLED module) |
| OLED GND | GND | Ground |
| Joystick GND | GND | Ground |
| Buzzer GND | GND | Ground |

## 2. Required Libraries

Install these via Arduino IDE Library Manager (Sketch → Include Library → Manage Libraries):

1. **Adafruit GFX Library** by Adafruit
2. **Adafruit SSD1306** by Adafruit
3. **Wire.h** (built-in with Arduino IDE)
4. **EEPROM.h** (built-in with Arduino IDE)

## 3. Arduino IDE Installation Instructions

1. Download and install [Arduino IDE](https://www.arduino.cc/en/software) (or use Arduino IDE 2.x)
2. Open Arduino IDE
3. Install required libraries:
   - Go to Sketch → Include Library → Manage Libraries...
   - Search for "Adafruit GFX" and install
   - Search for "Adafruit SSD1306" and install
   - Wire.h and EEPROM.h are built-in, no installation needed
4. Connect your Arduino Nano to computer via USB
5. In Arduino IDE:
   - Tools → Board → "Arduino Nano"
   - Tools → Processor → "ATmega328P (Old Bootloader)"
   - Tools → Port → Select your Arduino Nano port
6. Open the ArduinoVirtualPet.ino file
7. Click the Upload button (right-arrow icon)

## 4. How to Compile/Upload to Arduino Nano

1. Verify all connections match the pin mapping above
2. Select correct board and processor in Tools menu
3. Select correct COM port
4. Click the Verify button (checkmark) to compile
5. If compilation successful, click Upload button (right-arrow)
6. Wait for upload to complete (you'll see "Done uploading" message)
7. The OLED should display the startup sequence: "MY PET IS READY!"

## 5. Controls Explanation

- **Joystick UP**: Move menu selection upward
- **Joystick DOWN**: Move menu selection downward  
- **Joystick LEFT**: Back/cancel (returns to main screen)
- **Joystick RIGHT**: Select/open menu item
- **Joystick Press (click)**: Confirm/action (varies by context)

### Context-Specific Controls:

**Main Screen**:
- RIGHT: Open action menu
- LEFT: View status screen (alternate view)
- UP/DOWN: Cycle through quick action hints
- Press: Perform suggested action

**Menu Screen**:
- UP/DOWN: Navigate menu options
- RIGHT: Select highlighted option
- LEFT/BACK: Return to main screen
- Press: Same as RIGHT (select)

**Feeding Screen**:
- Press: Feed pet (increases hunger/happiness)
- Automatic exit after feeding or timeout

**Playing Screen (Catch Game)**:
- Joystick: Move cursor to catch falling targets
- Press: Attempt to catch target when cursor overlaps
- Automatic exit when time runs out or after restart

**Cleaning Screen**:
- Press: Clean pet (increases cleanliness)
- Automatic exit after cleaning or timeout

**Sleeping Screen**:
- Press: Wake pet up
- Automatic energy restoration while sleeping

**Status Screen**:
- Press/LEFT/BACK: Return to main screen

**Game Over Screen**:
- Press: Restart game (resets to main screen)

## 6. Game Systems Explanation

### Core Stats System
- **Hunger** (0-100): Decreases over time; low hunger reduces health
- **Happiness** (0-100): Decreases slowly; low happiness slightly reduces health
- **Energy** (0-100): Decreases when awake; recovers when sleeping
- **Health** (0-100): Decreases from neglect (hunger, cleanliness, happiness); game over at 0
- **Cleanliness** (0-100): Decreases over time; low cleanliness reduces health
- **Age**: Increases over time (configurable via AGE_INTERVAL_MS)
- **Experience**: Earned from all activities; used for leveling
- **Level**: Increases when experience threshold reached; increases XP needed for next level

### Pet Needs & Interactions
- **Feeding**: Increases hunger (+25) and happiness (+10), grants 5 XP
- **Playing**: Catch game awards happiness (+15 per catch), experience (+10 per catch + score bonus), costs energy
- **Cleaning**: Increases cleanliness (+20) and happiness (+5), grants 3 XP
- **Sleeping**: Gradually restores energy while sleeping, grants small XP over time
- **Aging**: Pet ages over time; awards 1 XP per age increment
- **Health Damage**: Occurs when hunger, cleanliness, or happiness fall below 20

### Leveling System
- XP required for next level = BASE × LEVEL × FACTOR (15 × level × 2)
- Level up triggers celebration sound and visual feedback
- XP carries over between levels
- Maximum level: 99

### Mini-Game: Catch Game
- Target appears randomly on screen
- Player moves cursor with joystick to intercept target
- Press button to attempt catch when overlapping
- Successful catch increases score, happiness, and experience
- Game lasts 10 seconds or until player stops
- Energy cost for playing

### Audio System
- **Sound Effects**: Button clicks, menu navigation, eating, catching, bubbles, level up, game over, wake
- **Blocking**: Uses tone() function with short durations to minimize blocking
- **SFX Muting**: Can be disabled by setting sfxEnabled = false
- **Frequency Range**: Uses audible frequencies suitable for small buzzer

### Save System
- **EEPROM Storage**: Saves all pet stats to prevent data loss on power cycle
- **Save Throttling**: Limited to every 30 seconds minimum to preserve EEPROM life
- **Versioning**: Includes save version to handle future updates
- **Auto-Save**: Occurs periodically and after significant actions
- **Load/Initialize**: On boot, loads saved data or initializes new pet

### Visual Design
- **Status Bar**: Top 16 pixels (yellow section on OLED) shows HP and Hunger
- **Gameplay Area**: Remaining 48 pixels (blue section) shows pet, animations, menus
- **Pet Expressions**: Different bitmaps for idle, happy, sleeping, sick states
- **Animations**: Simple bitmap swapping for expressions
- **UI Elements**: Menu cursor, status displays, game graphics, icons

## 7. Arduino Nano Limitations

Given the ATmega328P constraints:
- **RAM**: 2KB total (variables, stack, heap)
  - Current usage: ~1.5KB (leaves room for basic operation)
  - Major consumers: display buffer (1024 bytes), pet stats, buffers
- **Flash**: 32KB total (program storage)
  - Current usage: ~24KB (leaves room for enhancements)
- **EEPROM**: 1KB (100,000 write cycles)
  - Used for save system with wear leveling via throttling
- **Clock Speed**: 16MHz
  - Limits complex calculations and animation frame rates
- **No Floating Point Hardware**: Uses integer math primarily

### Workarounds Implemented:
- Simple bitmap graphics instead of complex sprites
- Throttled stat updates (every 5 seconds) instead of every loop
- Limited animation frames to conserve memory
- EEPROM write throttling to extend lifespan
- Fixed-point arithmetic for stats
- Efficient display updates only when needed

## 8. Troubleshooting Guide

### Blank OLED
- **Check Wiring**: SDA→A4, SCL→A5, VCC→3.3V/5V, GND→GND
- **I2C Address**: Try 0x3D if 0x3C doesn't work (some modules differ)
- **Power**: Ensure OLED receives proper voltage (check module specs)
- **Contrast**: Some modules need contrast adjustment (not implemented in this code)
- **Reset**: Try unplugging and replugging USB power

### Wrong OLED Address
- The code uses 0x3C; common alternatives are 0x3D
- To find your address, run an I2C scanner sketch:
  ```cpp
  #include <Wire.h>
  void setup() {
    Wire.begin();
    Serial.begin(9600);
    while (!Serial); // Wait for serial monitor
    Serial.println("I2C Scanner");
  }
  void loop() {
    byte error, address;
    int nDevices = 0;
    for(address = 1; address < 127; address++ ) {
      Wire.beginTransmission(address);
      error = Wire.endTransmission();
      if (error == 0) {
        Serial.print("I2C device found at address 0x");
        if (address<16) Serial.print("0");
        Serial.print(address,HEX);
        Serial.println("  !");
        nDevices++;
      }
    }
    if (nDevices == 0) Serial.println("No I2C devices found\n");
    else Serial.println("done\n");
    delay(5000);
  }
  ```
- Change `OLED_ADDRESS` constant to match your scanner result

### Joystick Reversed
- If up/down feels reversed, swap VRX and VRY connections
- Or modify the code: swap `joyX` and `joyY` assignments in `updateInput()`
- If left/right feels reversed, similarly swap or invert the logic

### Joystick Drifting
- **Adjust Deadzone**: Increase `JOY_DEADZONE` value (try 150-200)
- **Calibrate**: Note center values when joystick is at rest, adjust comparisons
- **Hardware**: Some KY-023 modules have inherent drift; this is normal
- **Software Fix**: Implement running average or calibration routine

### Button Not Working
- **Check Connection**: Ensure SW pin connects to D3
- **Pull-Up**: Code uses `INPUT_PULLUP`; verify wiring matches
- **Ground**: Confirm joystick ground connects to Arduino GND
- **Test**: Use simple button test sketch to verify D3 reads correctly
- **Debounce**: If too sensitive, increase `debounceDelay` value

### Buzzer Silent
- **Connection**: Verify buzzer connects to D9 and GND
- **Buzzer Type**: Ensure you have a *passive* buzzer (this code uses `tone()`)
- **Polarity**: Passive buzzers don't have polarity, but check connections
- **Volume**: Some buzzers are quiet; try placing near ear or using amplifier
- **Test**: Run simple tone test: `tone(9, 1000, 500);` in setup()

### Distorted Sound
- **Duration**: Keep tone durations short (<100ms) to avoid audible artifacts
- **Frequency**: Avoid extremely low or high frequencies that buzzer can't reproduce well
- **Clipping**: Don't layer multiple tones without proper mixing (not implemented)
- **Power**: Ensure stable power supply to avoid voltage fluctuations affecting buzzer
- **Muting**: The code prevents SFX spam with minimum interval timing

### EEPROM/Save Problems
- **Corruption**: If save data becomes invalid, code will reinitialize with default stats
- **Wear Leveling**: Saves limited to every 30 seconds minimum
- **Reset Save**: To force reinitialization, upload a sketch that clears EEPROM first:
  ```cpp
  #include <EEPROM.h>
  void setup() {
    for (int i = 0; i < EEPROM.length(); i++) {
      EEPROM.update(i, 0);
    }
  }
  void loop() {}
  ```
- **Version Mismatch**: If you modify the PetStats structure, increment `SAVE_VERSION`
- **Diagnostics**: Add Serial.print statements to save/load functions for debugging

### General Issues
- **Compilation Errors**: Ensure all libraries are installed correctly
- **Upload Failures**: Check correct board/processor/port selection; try different USB cable
- **Erratic Behavior**: Watch for loose connections or power supply issues
- **OLED Flickering**: Normal during updates; excessive flickering may indicate timing issues
- **Reset Loop**: If sketch constantly resets, check for memory issues or infinite loops

## 9. Notes for Optimal Experience

### Initial Setup
1. Upload the sketch and verify OLED shows startup sequence
2. Test all controls in menu system
3. Try each activity (food, play, clean, sleep) to see stat changes
4. Leave pet unattended to observe natural stat decay and aging

### Recommended Care Routine
- **Feed** when hunger drops below 50 (every few minutes)
- **Play** when happiness drops below 60 (for fun and XP)
- **Clean** when cleanliness drops below 50 or after playing
- **Sleep** when energy drops below 40 (especially overnight)
- **Check Status** regularly to monitor all stats
- **Enjoy** watching your pet grow older and level up!

### Advanced Tips
- The pet gains XP from simply surviving and aging
- Leveling up becomes progressively harder (more XP needed)
- Different activities provide different stat balances
- Neglecting multiple stats simultaneously compounds health damage
- The catch game rewards skill with higher happiness/XP gains
- Pet expresses different moods based on current stat balance

### Customization Options
- **Aging Speed**: Adjust `AGE_INTERVAL_MS` (lower = faster aging)
- **Stat Decay**: Modify the decay amounts in `updatePet()` function
- **XP Rewards**: Change experience values in various handler functions
- **Sound**: Modify frequencies/durations in `playSFX()` function
- **Graphics**: Edit bitmap arrays for different pet expressions
- **Menu Items**: Change `menuItems` array and add corresponding handlers

## Conclusion

This implementation provides a complete Tamagotchi-style virtual pet experience on Arduino Nano. The pet has evolving needs, responds to care, levels up, persists between power cycles via EEPROM save system, and provides engaging interactions through menus and a catch game mini-game.

While constrained by the Arduino Nano's limited resources, the game prioritizes:
- Polished UI with status bar in yellow OLED section
- Cute pet animations and expressions
- Responsive controls with proper debouncing
- Engaging sound effects via passive buzzer
- Balanced pet mechanics that encourage regular care
- Persistent save data that survives power loss

Enjoy nurturing your digital companion!