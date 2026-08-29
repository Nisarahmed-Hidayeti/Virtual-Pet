# Control Test Guide for Arduino Virtual Pet

This guide helps you test and verify that all controls are working correctly in the Arduino Virtual Pet project.

## Testing Procedure

### 1. Initial Startup
- Power on or reset the Arduino Nano
- OLED should display:
  - Line 1: "MY PET" (large text)
  - Line 2: "READY!" (large text)
  - After ~2 seconds: Main game screen

### 2. Main Screen Controls
On the main screen (showing pet, HP, Hunger, Level):
- **Joystick LEFT**: Should navigate to Status screen (showing detailed stats)
- **Joystick RIGHT**: Should open the main menu
- **Joystick UP/DOWN**: May show hints or have no effect (optional)
- **Joystick Press**: May have no effect or trigger a default action

### 3. Menu Screen Controls
When the menu is visible (list of options: FOOD, PLAY, CLEAN, SLEEP, CARE, STATUS):
- **Joystick UP**: Move selection highlight upward (wrap from top to bottom)
- **Joystick DOWN**: Move selection highlight downward (wrap from bottom to top)
- **Joystick RIGHT**: Select the highlighted menu item
- **Joystick Press**: Same as RIGHT (select highlighted item)
- **Joystick LEFT**: Return to main screen without selecting

### 4. Menu Item Testing
Test each menu item individually:

#### FOOD Menu Item
- Selecting FOOD should show "FEEDING..." screen briefly
- After ~1 second, return to main screen
- Verify: Hunger increases by 25, Happiness increases by 10

#### PLAY Menu Item
- Selecting PLAY should start the catch game:
  - Screen shows: Cursor (blinking square), Target (inverse square), Score, Timer
- **Joystick**: Move cursor around screen
- **Joystick Press**: Attempt to catch target when overlapping
- Game lasts 10 seconds or until exited
- After game: Return to main screen
- Verify: Successful catches increase Happiness and Experience

#### CLEAN Menu Item
- Selecting CLEAN should show "CLEANING..." screen briefly
- After ~1 second, return to main screen
- Verify: Cleanliness increases by 20, Happiness increases by 5

#### SLEEP Menu Item
- Selecting SLEEP should show "SLEEPING..." screen
- While sleeping: Energy gradually increases (shown by slow gain)
- **Joystick Press**: Wake up immediately and return to main screen
- Verify: Energy increases while sleeping

#### CARE Menu Item
- Selecting CARE should navigate to Status screen (same as LEFT from main)
- Shows detailed statistics: HP, Hunger, Happy, Energy, Clean, Age, Lv, XP
- **Joystick Press** or **LEFT**: Return to main screen

#### STATUS Menu Item
- Same as CARE - navigates to Status screen

### 5. Status Screen Controls
From the Status screen (detailed stats):
- **Joystick Press**: Return to main screen
- **Joystick LEFT**: Return to main screen
- **Joystick RIGHT**: May have no effect
- **Joystick UP/DOWN**: May have no effect

### 6. Special States
Test these states occur naturally:

#### Game Over Screen
- When pet's Health reaches 0:
  - Screen shows: "GAME OVER" and "Press to restart"
  - **Joystick Press**: Reset pet and return to main screen

### 7. Control Sensitivity Adjustments
If controls feel too sensitive or not sensitive enough:
- **JOY_DEADZONE** (line 27): Increase value (try 100-150) to reduce sensitivity
- **debounceDelay** (line 94): Increase value (try 75-100) for more stable button reading

### 8. Troubleshooting Common Issues

#### "Menu selection doesn't respond to button press"
- Check wiring: Joystick SW pin to D3
- Verify button is active low (wired correctly with pull-up)
- Increase debounceDelay if button bounces too much
- Ensure joyButtonPressed is properly reset after use

#### "Only see half the screen"
- Verify OLED initialization tries both 0x3C and 0x3D addresses
- Check Wire library is included
- Verify OLED connections: SDA→A4, SCL→A5
- Try different OLED_ADDRESS constant if needed

#### "Joystick movement feels reversed"
- Swap VRX and VRY connections (A0 and A1)
- OR modify handleInput(): swap joyX and joyY assignments
- For left/right reversal: invert the joyX comparisons

#### "No sound from buzzer"
- Verify buzzer connected to D9 and GND
- Ensure using passive buzzer (active buzzers won't work with tone())
- Test with: tone(9, 1000, 500); in setup()
- Check buzzer orientation (passive typically non-polarized)

#### "Controls work but feel laggy"
- Reduce delay values in handleInput() (try 100ms instead of 150/200)
- Consider removing some delays if input feels sluggish
- The delay(10) in loop() provides basic timing

## Expected Behavior Summary
- All menu items accessible and selectable
- Button press and joystick RIGHT both work for selection
- LEFT consistently returns/back/cancels
- UP/DOWN smoothly navigate menus
- Each state has clear entry and exit points
- No stuck states or uncontrolled transitions