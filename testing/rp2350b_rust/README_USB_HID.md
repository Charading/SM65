# USB HID Keyboard for RP2350B

This project implements a USB HID keyboard device using Rust for the RP2350B microcontroller.

## Features

- USB HID keyboard device
- Sends periodic 'a' key presses (press/release every 500ms)
- Uses `usb-device` and `usbd-hid` crates
- Compatible with both RP2040 and RP2350 targets

## Dependencies

The project uses:
- `rp235x-hal` - Hardware Abstraction Layer for RP2350
- `usb-device` v0.3 - USB device framework
- `usbd-hid` v0.7 - HID class implementation

## Building

### Release build (optimized):
```powershell
cargo build --release
```

### Debug build:
```powershell
cargo build
```

The compiled binary will be at:
- Release: `target/thumbv8m.main-none-eabihf/release/rp2350b_rust`
- Debug: `target/thumbv8m.main-none-eabihf/debug/rp2350b_rust`

## Flashing

Use `picotool` or your preferred method to flash the `.elf` file to your RP2350B board:

```powershell
# Using the Run Project task (automatically loads via picotool)
# Or manually:
picotool load -x target/thumbv8m.main-none-eabihf/release/rp2350b_rust -t elf
```

## How It Works

1. **Initialization**: The code initializes the RP2350B clocks, USB peripheral, and creates a USB HID keyboard device.

2. **USB Setup**: 
   - Creates a USB device with VID 0x1209, PID 0x0001 (test IDs)
   - Configures as a HID keyboard class device
   - Initializes the USB bus allocator with DPRAM and USB clock

3. **Main Loop**:
   - Polls the USB device for events
   - Alternates between sending:
     - Key press report (HID usage ID 4 = 'a' key)
     - Key release report (all zeros)
   - 500ms delay between toggles

## USB VID/PID

The current implementation uses test VID/PID values:
- **VID**: 0x1209 (pid.codes test VID)
- **PID**: 0x0001

For production, you should obtain your own USB VID/PID or use an officially allocated test ID.

## Platform Support

The code conditionally compiles for:
- **RP2040**: Uses `rp2040-hal` and simpler USB bus initialization
- **RP2350**: Uses `rp235x-hal` with DPRAM and USB clock requirements

## Modifying the Keyboard

To change the key being sent, modify the `keycodes` array in `src/main.rs`:

```rust
keycodes: [4, 0, 0, 0, 0, 0], // HID usage ID 4 == 'a'
```

Common HID usage IDs:
- 4 = 'a'
- 5 = 'b'
- 6 = 'c'
- 30 = '1'
- 31 = '2'
- etc.

See the [USB HID Usage Tables](https://usb.org/sites/default/files/hut1_22.pdf) for complete reference.

## Troubleshooting

- **Build errors**: Ensure you have the correct Rust target installed:
  ```powershell
  rustup target add thumbv8m.main-none-eabihf
  ```

- **Device not recognized**: Check that your board is in BOOTSEL mode when flashing

- **No keypresses detected**: Verify the USB connection and that the device enumerates as a HID keyboard in your OS device manager

## License

This project follows the same license as the rp-rs examples (MIT OR Apache-2.0).
