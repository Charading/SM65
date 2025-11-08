# RP2350B ADC Keyboard Firmware

A custom firmware for RP2350B-based keyboard using ADC for Hall effect or analog key detection with rotary encoder support and WS2812 RGB LEDs.

## Features

### ✨ Core Features
- **8-Channel ADC Key Detection**: Detects key presses based on 10% deviation from calibrated baseline (RP2350B has native 8 ADC channels!)
- **Rotary Encoder**: Volume control (rotation) and mute (button press)
- **USB HID Keyboard**: Emulates standard keyboard with keys 0-7
- **USB CDC Serial**: Real-time ADC value monitoring and debugging
- **WS2812 RGB LEDs**: 8 LEDs with visual feedback for key states
- **VIA/SignalRGB Ready**: Basic framework for RGB customization (expandable)

## Hardware Configuration

### ADC Channels (Keys 0-7) - RP2350B
The RP2350B variant has **8 ADC channels** built-in:
- **ADC0** → GP26 (Key 0)
- **ADC1** → GP27 (Key 1)
- **ADC2** → GP28 (Key 2)
- **ADC3** → GP29 (Key 3)
- **ADC4** → GP40 (Key 4)
- **ADC5** → GP41 (Key 5)
- **ADC6** → GP42 (Key 6)
- **ADC7** → GP43 (Key 7)

> **Note**: The RP2350B (QFN-80 package) has all 8 ADC channels. Perfect for your design!

### Rotary Encoder
- **CLK** → GP22
- **DT** → GP21
- **SW** → GP20 (button)

### WS2812 LEDs
- **Data** → GP28 (⚠️ conflicts with ADC2) or GP16 (recommended)
- **Count**: 8 LEDs

> **Important**: GP28 is used for both ADC2 and WS2812 in the default config. Recommended to move WS2812 to GP16 by editing `LED_PIN` in `led.h`.

## Functionality

### Key Detection (ADC)
1. **Calibration**: On startup, the system samples all ADC channels 100 times to establish baseline values
2. **Detection**: Monitors ADC values continuously; when any channel deviates by ±10% from baseline, a key press is registered
3. **Key Mapping**: 
   - ADC Channel 0 → Number 0
   - ADC Channel 1 → Number 1
   - ... and so on

### Encoder Controls
- **Rotate Clockwise**: Volume Up
- **Rotate Counter-Clockwise**: Volume Down
- **Press Button**: Mute Toggle

### LED Feedback
- **During Calibration**: All LEDs yellow
- **Key Pressed**: LED turns green
- **Key Released**: LED turns dim purple/off

### Serial Interface
- **Baud Rate**: USB CDC (full speed)
- **Output**: Real-time ADC values every ~100ms
  ```
  ADC: CH0=1234(1200) CH1=2345(2300) CH2=3456(3400) ...
  ```
- **Format**: Current value (Baseline value)

## Building the Project

### Prerequisites
- Raspberry Pi Pico SDK 2.2.0+
- CMake 3.13+
- ARM GCC toolchain
- VS Code with Raspberry Pi Pico extension (recommended)

### Build Steps

#### Using VS Code Tasks
1. Open the workspace in VS Code
2. Press `Ctrl+Shift+B` or run "Compile Project" task
3. Output: `build/rp2350_firmware_testing.uf2`

#### Manual Build
```bash
cd build
cmake ..
make
```

### Flashing

#### Method 1: UF2 Boot Mode
1. Hold BOOTSEL button while connecting USB
2. Copy `rp2350_firmware_testing.uf2` to the drive
3. Device will reboot automatically

#### Method 2: Using picotool (from VS Code)
Run the "Run Project" task

#### Method 3: Using OpenOCD (from VS Code)
Run the "Flash" task (requires debugger)

## Usage

1. **Connect USB**: Device will enumerate as:
   - HID Keyboard
   - CDC Serial Port
   
2. **Calibration**: 
   - On first power-up, ensure no keys are pressed
   - LEDs will turn yellow during calibration (0.5s)
   - LEDs turn off when ready

3. **Testing**:
   - Open serial monitor (115200 baud) to see ADC values
   - Press keys to trigger key presses (numbers 0-7)
   - Rotate encoder to adjust volume
   - Press encoder button to mute

## Code Structure

```
rp2350_firmware_testing/
├── rp2350_firmware_testing.c  # Main application
├── adc.c / adc.h              # ADC calibration & key detection
├── encoder.c / encoder.h      # Rotary encoder handling
├── usb.c / usb.h              # USB HID keyboard & consumer control
├── usb_descriptors.c          # USB device descriptors
├── serial.c / serial.h        # USB CDC serial interface
├── led.c / led.h              # WS2812 LED control (PIO)
├── tusb_config.h              # TinyUSB configuration
└── CMakeLists.txt             # Build configuration
```

## Customization

### Adjusting Sensitivity
Edit `adc.h`:
```c
#define ADC_DEVIATION_THRESHOLD 0.10f  // 10% = 0.10
```

### Changing Key Mappings
Edit `rp2350_firmware_testing.c`:
```c
static const uint8_t number_keycodes[8] = {
    0x27, // 0
    0x1E, // 1
    // ... modify as needed
};
```

HID keycodes: [USB HID Usage Tables](https://www.usb.org/sites/default/files/documents/hut1_12v2.pdf)

### LED Colors
Edit `led.c` in `led_update_keys()`:
```c
// Key pressed
led_buffer[i].r = 0;
led_buffer[i].g = 50;  // Brightness
led_buffer[i].b = 0;
```

### Adding More ADC Channels
All 8 channels are already implemented for RP2350B! Just wire up your Hall effect sensors to GP26, GP27, GP28, GP29, GP40, GP41, GP42, GP43.

## VIA and SignalRGB Support

Basic framework is in place in `led.c`. To fully implement:

### VIA Support
1. Add Raw HID endpoint to USB descriptors
2. Implement VIA protocol command parsing
3. Add EEPROM storage for settings
4. Create VIA JSON definition file

### SignalRGB Support
1. Implement SignalRGB protocol in `led_process_via_command()`
2. Add RGB effects library
3. Handle SignalRGB discovery packets

## Troubleshooting

### Keys Not Responding
- Check serial output for ADC values
- Verify baseline calibration completed
- Adjust `ADC_DEVIATION_THRESHOLD` if too sensitive/insensitive

### Encoder Not Working
- Verify pin connections (CLK=GP22, DT=GP21, SW=GP20)
- Check pull-ups are enabled
- Test with serial output

### LEDs Not Working
- Verify WS2812 data pin on GP28
- Check 5V power supply to LEDs
- Ensure LED count is correct (8)

### USB Not Enumerating
- Check TinyUSB configuration
- Verify USB cable supports data
- Try different USB port
- Check serial output for initialization messages

## License

This is experimental firmware for testing purposes. Feel free to modify and use for your own projects.

## Credits

- Built with [Raspberry Pi Pico SDK](https://github.com/raspberrypi/pico-sdk)
- USB stack: [TinyUSB](https://github.com/hathach/tinyusb)
- WS2812 control using RP2350 PIO

## Future Enhancements

- [ ] Full VIA support
- [ ] SignalRGB integration
- [ ] RGB effects library
- [ ] EEPROM settings storage
- [ ] Key mapping customization
- [ ] Macro support
- [ ] 8-channel ADC with multiplexer
- [ ] QMK-style configuration
