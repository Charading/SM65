# Quick Start Guide

## 🚀 Getting Started in 5 Minutes

### 1. Flash the Firmware
1. Hold **BOOTSEL** button on your RP2350 board
2. Connect USB cable to computer
3. Release BOOTSEL - board appears as USB drive
4. Copy `build/rp2350_firmware_testing.uf2` to the drive
5. Board automatically reboots with new firmware

### 2. Connect Your Hardware

#### Minimum Setup (Testing)
Just connect USB - the firmware will work but show errors for missing hardware.

#### Full Setup
```
Encoder:
  CLK → GP22
  DT  → GP21  
  SW  → GP20
  GND → GND

LEDs (WS2812):
  DIN → GP28
  VCC → 5V
  GND → GND

ADC Keys (Hall Effect/Analog):
  Key 0 → GP26 (ADC0)
  Key 1 → GP27 (ADC1)
  Key 2 → GP28 (ADC2) ⚠️ Conflicts with LED!
  Key 3 → GP29 (ADC3)
  Key 4 → GP40 (ADC4)
  Key 5 → GP41 (ADC5)
  Key 6 → GP42 (ADC6)
  Key 7 → GP43 (ADC7)
```

**⚠️ Pin Conflict Warning**: GP28 is used for both ADC2 and WS2812 data. **Recommended solution**:
- Edit `led.h`: Change `#define LED_PIN 28` to `#define LED_PIN 16`
- Wire WS2812 data to GP16 instead of GP28
- This allows all 8 ADC channels to work!

### 3. Test It

#### Serial Monitor
1. Open Serial Monitor (115200 baud)
2. You should see:
   ```
   RP2350 ADC Keyboard Initializing...
   Calibrating ADC...
   Calibration complete!
   System ready.
   
   ADC: CH0=1234(1200) CH1=2345(2300) ...
   ```

#### Test Keys
- Touch/actuate Hall effect sensors
- Watch serial output for "Key X pressed"
- LEDs should light up green
- Computer should register number keys 0-7

#### Test Encoder
- Rotate encoder → Volume changes
- Press encoder → Mute toggles
- Check serial output for confirmation

## 🔧 Customization

### Change Key Sensitivity
Edit `config.h`:
```c
#define ADC_DEVIATION_PERCENT   10  // Change to 5-20
```
- Lower = more sensitive (triggers easier)
- Higher = less sensitive (requires more actuation)

### Change Key Mappings  
Edit `config.h` - uncomment alternative keycode sets:
```c
// Numbers 0-7 (default)
// Letters A-H
// Function keys F1-F8
```

### Change LED Colors
Edit `config.h`:
```c
#define LED_COLOR_KEY_PRESSED_R   0
#define LED_COLOR_KEY_PRESSED_G   50
#define LED_COLOR_KEY_PRESSED_B   0
```

### Adjust LED Brightness
Edit `config.h`:
```c
#define LED_BRIGHTNESS_MAX      50  // 0-255
```
Lower values reduce power consumption.

## 📊 Serial Commands

Currently, the serial interface outputs data only. To add commands:

1. Edit `serial.c` → `serial_task()`
2. Parse incoming bytes
3. Add command handlers

Example commands you could add:
- `cal` - Re-calibrate ADC
- `led <r> <g> <b>` - Set LED color
- `sens <value>` - Change sensitivity
- `info` - Print system info

## 🐛 Troubleshooting

### No Serial Output
- Check USB cable (must support data)
- Try different USB port
- Wait 2-3 seconds after connecting
- Windows: Check Device Manager for COM port

### Keys Not Working
1. Open serial monitor
2. Check if ADC values are changing
3. If values stuck at 0 - check ADC pin connections
4. If values changing but no key press - adjust `ADC_DEVIATION_PERCENT`

### LEDs Not Working
1. Check WS2812 connections (DIN, VCC, GND)
2. Verify 5V power supply
3. Some WS2812 need level shifter for 3.3V → 5V logic
4. Try changing `LED_GPIO_PIN` in case of pin conflict

### Encoder Not Responding
1. Check pins: CLK=GP22, DT=GP21, SW=GP20
2. Verify encoder has pull-ups (internal pull-ups are enabled)
3. Watch serial output while rotating
4. Some encoders are directionally sensitive - swap CLK/DT

### USB Not Recognized
1. Try different USB cable
2. Check if BOOTSEL mode works (indicates hardware OK)
3. Verify TinyUSB compiled correctly
4. Check VID/PID in `usb_descriptors.c` (some systems block 0xFEED)

## 📁 Project Structure

```
Core Files:
  rp2350_firmware_testing.c  ← Main application
  config.h                   ← Easy configuration

Modules:
  adc.c/h                    ← Key sensing
  encoder.c/h                ← Rotary encoder
  usb.c/h                    ← HID keyboard
  led.c/h                    ← RGB LEDs
  serial.c/h                 ← Debug output

USB:
  usb_descriptors.c          ← Device descriptors
  tusb_config.h              ← TinyUSB config
```

## 🎯 Next Steps

### For 8 Full Keys
Add analog multiplexer (CD4051):
1. Connect 8 analog sensors to multiplexer inputs
2. Connect multiplexer output to one ADC input
3. Use 3 GPIOs for S0, S1, S2 selection
4. Modify `adc.c` to cycle through channels

### For VIA Support
1. Add Raw HID endpoint to `usb_descriptors.c`
2. Implement VIA protocol in `led.c`
3. Create VIA JSON definition
4. Add EEPROM storage for settings

### For More Features
- Macros (detect key combo, send sequence)
- Layers (shift-like functionality)
- OLED display (show status)
- Battery monitoring (if portable)
- Bluetooth support (if using Pico W)

## 📚 Resources

- [RP2350 Datasheet](https://datasheets.raspberrypi.com/rp2350/rp2350-datasheet.pdf)
- [Pico SDK Docs](https://www.raspberrypi.com/documentation/pico-sdk/)
- [USB HID Usage Tables](https://www.usb.org/sites/default/files/documents/hut1_12v2.pdf)
- [VIA Documentation](https://www.caniusevia.com/docs/specification)
- [WS2812 Datasheet](https://cdn-shop.adafruit.com/datasheets/WS2812.pdf)

## 🎉 Success!

If you see this in your serial monitor, everything is working:
```
RP2350 ADC Keyboard Initializing...
Calibrating ADC...
Calibration complete!
System ready.

ADC: CH0=1234(1200) CH1=2345(2300) CH2=3456(3400) CH3=4567(4500) ...
```

Now start actuating keys and enjoy your custom keyboard! 🎮⌨️
