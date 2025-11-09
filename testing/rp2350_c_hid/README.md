# RP2350 USB HID Keyboard Enumeration

This project demonstrates how to enumerate a USB HID keyboard using the RP2350 microcontroller with the Pico SDK and TinyUSB.

## Features

- USB HID keyboard device enumeration
- Automatic keyboard device registration with the host system
- Button-triggered keystroke generation (sends 'E' key when GP30 is pressed)
- 5x HC4067 multiplexer ADC scanning (80 analog channels total)
- Periodic ADC value reporting via UART
- LED indication for USB connection status
- UART debug output

## Hardware Required

- Raspberry Pi Pico 2 with RP2350B chip
- 5x HC4067 16-channel analog multiplexers
- USB connection to host computer
- Button or switch connected between GP30 and ground
- UART connection for ADC data output (pins GP0/GP1)

## Building

The project is configured to build with the Pico SDK. Make sure you have the Pico SDK and toolchain installed.

1. Use VS Code with the Raspberry Pi Pico extension
2. Run the "Compile Project" task
3. The output files will be in the `build/` directory:
   - `rp2350_c_hid.uf2` - Flash this to your Pico 2
   - `rp2350_c_hid.elf` - ELF executable for debugging

## Flashing

1. Hold the BOOTSEL button while connecting the Pico 2 to your computer
2. Copy `rp2350_c_hid.uf2` to the RPI-RP2 drive that appears
3. The Pico will reboot and start running the program

## Usage

Once flashed and running:

1. The RP2350B will enumerate as a USB HID keyboard on your system
2. The onboard LED will blink slowly (1Hz) while connecting
3. Once enumerated, the LED will blink faster (4Hz) 
4. Press a button connected between GP30 and ground to send the 'E' key
5. Debug messages are sent over UART at 115200 baud on pins GP0 (TX) and GP1 (RX)

## Hardware Connections

### Button
- **GP30**: Connect to one side of a button/switch
- **GND**: Connect to the other side of the button/switch
- The internal pull-up resistor is enabled, so no external resistor is needed

### HC4067 Multiplexers
- **Select Lines** (shared across all 5 muxes):
  - S0 → GP10
  - S1 → GP11
  - S2 → GP12
  - S3 → GP13
- **Analog Outputs** (one per mux):
  - MUX1 → GP26 (ADC0)
  - MUX2 → GP27 (ADC1)
  - MUX3 → GP28 (ADC2)
  - MUX4 → GP29 (ADC3)
  - MUX5 → GP47 (ADC4) - RP2350B only
- **Power**: 3.3V to VCC, GND to GND
- **Enable**: Tie EN pins to GND for always-on operation
- **Analog Inputs**: Connect your sensors/signals to C0-C15 on each mux

## Code Structure

- `rp2350_c_hid.c` - Main application code
- `tusb_config.h` - TinyUSB configuration
- `CMakeLists.txt` - Build configuration

## Key Functions

- **USB Descriptors**: Device, configuration, string, and HID report descriptors
- **HID Callbacks**: Handle HID-specific USB requests
- **Keystroke Generation**: Sends test keystrokes to demonstrate functionality
- **Status Indication**: LED patterns show connection status

## Customization

You can modify the code to:
- Change the button pin (currently GP30) by modifying `BUTTON_PIN`
- Change the key sent (currently 'E') by modifying `HID_KEY_E`
- Add more buttons for different keys
- Implement key combinations or modifiers
- Add more complex HID functionality

## Troubleshooting

- Ensure UART stdio is enabled for debug output
- Check that the USB cable supports data (not power-only)
- Try different USB ports if enumeration fails
- Monitor debug output to see connection status

## License

This project uses the Pico SDK and TinyUSB, which are under their respective licenses.