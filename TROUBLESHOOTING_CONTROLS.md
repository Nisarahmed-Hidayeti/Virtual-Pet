# Troubleshooting Joystick and Button Controls

If you cannot enter the feeding screen or any other screens, follow this troubleshooting guide.

## Step 1: Verify Hardware Connections

Double-check your wiring matches this pin mapping:

| Component | Arduino Nano Pin | Wire Color (Suggestion) |
|-----------|------------------|-------------------------|
| OLED SDA | A4 | Green |
| OLED SCL | A5 | Yellow |
| Joystick VRX (X-axis) | A0 | Red |
| Joystick VRY (Y-axis) | A1 | White |
| Joystick SW (Button) | D3 | Orange |
| Passive Buzzer Signal | D9 | Brown |
| All GND | GND | Black |
| OLED VCC | 3.3V or 5V | Red |
| Buzzer GND | GND | Black |
| Joystick GND | GND | Black |

**Critical Checks:**
- Joystick VRX → A0 (NOT A1)
- Joystick VRY → A1 (NOT A0)
- Joystick SW → D3 (with internal pull-up enabled in code)
- Buzzer → D9 (PWM-capable pin)

## Step 2: Run the Control Debug Sketch

1. Upload `DEBUG_CONTROLS.ino` to your Arduino Nano
2. Open Serial Monitor (115200 baud)
3. You should see output like:
   ```
   === CONTROL DEBUG TEST ===
   Move joystick and press button
   Format: X/Y ButtonState
   Center should be around 0/0
   Range: -512 to +512
   Button: 1=pressed, 0=released
   ==========================
   -2/15 released
   8/-3 released
   ```

4. Test each control:
   - **Joystick Left/RIGHT**: X value should go negative (left) and positive (right)
   - **Joystick Up/DOWN**: Y value should go negative (up) and positive (down)
   - **Joystick Press**: Should show "PRESSED" when held down

## Step 3: Interpret Debug Results

### If Joystick Values Don't Change:
- Check VRX→A0 and VRY→A1 connections
- Verify joystick is getting power (VCC to 5V, GND to GND)
- Try a different joystick if available
- Check for loose wires or breadboard issues

### If Joystick Seems Reversed:
- If LEFT/RIGHT feels backwards: Try swapping VRX and VRY wires
- If UP/DOWN feels backwards: Reverse the wires OR invert in code
- Temporary fix in code: Swap `joyX` and `joyY` in `handleInput()`

### If Button Doesn't Register:
- Verify SW → D3 connection
- Try pressing button firmly
- Test with wire between D3 and GND (should read as PRESSED)
- Check if you accidentally wired to 5V instead of GND
- Remember: Button is ACTIVE LOW (pressed = LOW voltage = 0V)

## Step 4: Adjust Sensitivity Values

If you see movement but need to move joystick very far:
1. In `ArduinoVirtualPet.ino`, adjust `JOY_DEADZONE`:
   - Lower value = more sensitive (try 10, 5, or even 0)
   - Higher value = less sensitive (try 40, 50, 100)
2. Current value: `#define JOY_DEADZONE 20`

If button triggers multiple times:
1. Adjust `debounceDelay`:
   - Higher value = more debounce (try 75-100)
   - Current value: `const unsigned long debounceDelay = 50;`

## Step 5: Verify OLED is Working

Before debugging controls, ensure you can see:
1. Startup screen: "MY PET" then "READY!"
2. Main screen with pet image and stats
3. If OLED is blank or half-empty, check:
   - OLED_ADDRESS (try 0x3D if 0x3C doesn't work)
   - SDA→A4, SCL→A5 connections
   - OLED VCC to correct voltage (check your module)
   - Run an I2C scanner sketch

## Step 6: Test State Transitions Manually

If controls work but menus don't show:
1. Upload a minimal test sketch:
   ```cpp
   #include <Adafruit_SSD1306.h>
   #include <Wire.h>
   #define OLED_RESET -1
   Adafruit_SSD1306 display(128, 64, &Wire, OLED_RESET);
   
   void setup() {
     Wire.begin();
     display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
     display.display();
     delay(1000);
   }
   
   void loop() {
     display.clearDisplay();
     display.setTextSize(2);
     display.setCursor(10,10);
     display.print(F("TEST"));
     display.display();
     delay(500);
   }
   ```
2. If this works, the OLED and libraries are fine - focus on controls

## Step 7: Common Issues and Fixes

### Problem: Menu opens but selection doesn't change
- Check if `joyY` values are large enough to exceed deadzone
- Try reducing `JOY_DEADZONE` to 5 or 10
- Verify you're moving joystick UP/DOWN (not LEFT/RIGHT)

### Problem: Pressing button does nothing
- Confirm button wiring: one side to D3, other to GND
- Internal pull-up is enabled: `pinMode(JOY_BUTTON_PIN, INPUT_PULLUP)`
- Button should read HIGH when not pressed, LOW when pressed
- In code: `bool buttonReading = !digitalRead(JOY_BUTTON_PIN);`

### Problem: Always in menu or stuck in one state
- Check if joystick is drifting (constant offset)
- Try increasing deadzone to ignore drift
- Verify button isn't stuck pressed

## Step 8: Final Verification

Once controls work in debug sketch:
1. Note your typical joystick ranges (e.g., X: -200 to +180, Y: -150 to +220)
2. Note button behavior
3. Adjust `JOY_DEADZONE` if needed (should be less than your minimum movement)
4. Re-upload `ArduinoVirtualPet.ino`
5. Test menu navigation:
   - RIGHT should open menu
   - UP/DOWN should highlight different options
   - RIGHT or Press should select
   - LEFT should go back

**Remember**: The pet needs time to register movements - don't flick joystick quickly. Hold direction for 200ms for reliable detection.

If you still get stuck, reply with:
1. Your debug sketch output when moving joystick in each direction
2. What you see when pressing the button
3. Whether you see ANY screen changes when using controls