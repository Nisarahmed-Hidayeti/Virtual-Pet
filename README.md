# Arduino Nano Tamagotchi-Style Virtual Pet

A complete Tamagotchi-style virtual pet game built for Arduino Nano using the specified hardware.

## Hardware Requirements

- Arduino Nano (ATmega328P)
- 0.96" 128x64 OLED (SSD1306) via I2C
  - SDA → A4
  - SCL → A5
- KY-023 Joystick
  - VRX → A0
  - VRY → A1
  - SW → D3
- Passive Buzzer
  - Signal → D9
  - GND → GND

## Required Libraries

Install these libraries via Arduino Library Manager:
- Wire.h (built-in)
- Adafruit GFX Library
- Adafruit SSD1306
- EEPROM.h (built-in)

## Installation Instructions

1. Install the required libraries through Arduino IDE Library Manager
2. Open `ArduinoVirtualPet.ino` in Arduino IDE
3. Select Board: "Arduino Nano"
4. Select Processor: "ATmega328P (Old Bootloader)"
5. Connect your Arduino Nano via USB
6. Click Upload

## Controls

- **Joystick UP**: Move menu selection upward
- **Joystick DOWN**: Move menu selection downward
- **Joystick LEFT**: Back/cancel
- **Joystick RIGHT**: Select/open
- **Joystick Press**: Confirm/action

## Game Systems

### Pet Stats
- **Hunger** (0-100): Decreases over time, affects health when low
- **Happiness** (0-100): Decreases slowly over time
- **Energy** (0-100): Decreases when awake, recovers when sleeping
- **Health** (0-100): Decreases from neglect (low hunger, cleanliness, happiness)
- **Cleanliness** (0-100): Decreases over time, affected by activities
- **Age**: Increases over time (configurable rate)
- **Experience**: Earned from activities, used for leveling
- **Level**: Increases when experience threshold reached

### Main Menu
- **FOOD**: Feed the pet (increases hunger/happiness)
- **PLAY**: Play a mini-game (increases happiness/experience, decreases energy)
- **CLEAN**: Clean the pet (increases cleanliness)
- **SLEEP**: Put pet to sleep (recovers energy)
- **CARE**: Access status screen (shows detailed stats)
- **STATUS**: View detailed statistics

### Features
- Persistent save data using EEPROM (survives power loss)
- Non-blocking timing for smooth animations and responsive controls
- Sound effects via passive buzzer
- Simple chiptune melodies
- Pet animations and expressions
- Status bar in yellow OLED section
- Gameplay area in blue OLED section
- Level-up system with celebrations
- Game over when health reaches 0

## Limitations

Given Arduino Nano's constraints (2KB RAM, 32KB Flash):
- Simple bitmap graphics rather than complex sprites
- Limited animation frames
- Basic sound effects rather than complex music
- Stat updates occur at intervals rather than every loop
- EEPROM writes are throttled to prevent wear

## Troubleshooting

### Blank OLED
- Check wiring: SDA to A4, SCL to A5
- Verify I2C address (try 0x3D if 0x3C doesn't work)
- Ensure OLED is powered (VCC to 3.3V or 5V, GND to GND)

### Wrong OLED Address
- The code uses 0x3C; some modules use 0x3D
- Modify `OLED_ADDRESS` constant if needed
- Run an I2C scanner sketch to find your device's address

### Joystick Reversed
- If up/down or left/right feels reversed, swap the VRX/VRY connections
- Or modify the joyX/joyY reading logic in code

### Joystick Drifting
- Adjust `JOY_DEADZONE` constant (try values between 80-150)
- Calibrate by noting center values when joystick is at rest

### Button Not Working
- Ensure joystick SW pin is connected to D3
- Check that D3 is configured as INPUT_PULLUP
- Verify joystick ground is connected to Arduino GND

### Buzzer Silent
- Check buzzer connected to D9 and GND
- Verify buzzer is passive (not active) - this code uses tone()
- Test with simple tone example

### Distorted Sound
- Keep tone durations short (<100ms) to avoid blocking
- Volume depends on buzzer quality and voltage
- Avoid playing tones too frequently

### EEPROM/Save Problems
- EEPROM has limited write cycles (~100,000)
- Saves are throttled to every 30 seconds minimum
- If save data corrupts, EEPROM will reinitialize with defaults
- To reset save data, upload a sketch that clears EEPROM first

## Notes for Enjoyment

This implementation provides a solid foundation for a Tamagotchi-style experience. The pet has needs that change over time, responds to care, levels up, and persists between power cycles. For the fullest experience:

1. Feed your pet regularly when hungry
2. Play with it to increase happiness
3. Clean it after playing
4. Let it sleep when tired
5. Monitor the status screen for detailed stats
6. Enjoy the little animations and sound effects!

The yellow status bar shows key vitals, while the blue gameplay area displays the pet and animations. With proper care, your virtual pet will thrive and grow!

---

*Created with Claude Code*