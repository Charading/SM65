# Pin Configuration Reference

## RP2350B Pin Assignments

| Function | GPIO Pin | Description |
|----------|----------|-------------|
| **ADC Inputs (RP2350B has 8!)** | | |
| ADC Channel 0 (Key 0) | GP26 | Analog input for key 0 |
| ADC Channel 1 (Key 1) | GP27 | Analog input for key 1 |
| ADC Channel 2 (Key 2) | GP28 | Analog input for key 2 |
| ADC Channel 3 (Key 3) | GP29 | Analog input for key 3 |
| ADC Channel 4 (Key 4) | GP40 | Analog input for key 4 |
| ADC Channel 5 (Key 5) | GP41 | Analog input for key 5 |
| ADC Channel 6 (Key 6) | GP42 | Analog input for key 6 |
| ADC Channel 7 (Key 7) | GP43 | Analog input for key 7 |
| **Rotary Encoder** | | |
| Encoder CLK | GP22 | Encoder clock signal |
| Encoder DT | GP21 | Encoder data signal |
| Encoder SW | GP20 | Encoder switch (button) |
| **RGB LEDs** | | |
| WS2812 Data | GP28 | Data line for 8x WS2812 LEDs |
| **USB** | | |
| USB D+ | GP0 | USB Data Plus (hardware) |
| USB D- | GP1 | USB Data Minus (hardware) |

## Notes

### RP2350B Advantage
The RP2350B package has **8 ADC channels** (ADC0-7), perfect for your 8-key design! No multiplexer needed.

### WS2812 LED Wiring
- **VCC**: Connect to 5V power supply
- **GND**: Common ground with RP2350B
- **DIN**: Connect to GP28
- **DOUT**: Chain to next LED (8 total)

**⚠️ Pin Conflict**: GP28 is used for both ADC2 and WS2812 LEDs. Options:
- **Recommended**: Move WS2812 to a different GPIO (e.g., GP16, GP17, GP18)
- **Alternative**: Don't use ADC2, only use 7 keys
- **Note**: You can change `LED_PIN` in `led.h` to any available GPIO

### Rotary Encoder Wiring
- **CLK**: GP22 with internal pull-up enabled
- **DT**: GP21 with internal pull-up enabled  
- **SW**: GP20 with internal pull-up enabled (active low)
- **GND**: Common ground
- **+**: Optional 3.3V if encoder needs power

## Power Considerations

- **RP2350**: 3.3V logic levels
- **WS2812 LEDs**: 
  - Logic: 5V (but will work with 3.3V in most cases)
  - Power: 5V required
  - Current: ~60mA per LED at full brightness
  - Total: ~480mA for 8 LEDs at full white
- **USB Power**: 500mA max, budget accordingly

## Alternative Configurations

### Recommended: Move WS2812 to GP16
To avoid the GP28 conflict between ADC2 and LEDs:
1. Wire WS2812 data to GP16 instead of GP28
2. Edit `led.h`: Change `#define LED_PIN 28` to `#define LED_PIN 16`
3. Recompile
4. Now all 8 ADC channels are available!

### Pin Layout Without Conflicts
```
ADC Inputs:
  GP26 → ADC0 (Key 0)
  GP27 → ADC1 (Key 1)
  GP28 → ADC2 (Key 2)
  GP29 → ADC3 (Key 3)
  GP40 → ADC4 (Key 4)
  GP41 → ADC5 (Key 5)
  GP42 → ADC6 (Key 6)
  GP43 → ADC7 (Key 7)

WS2812 LEDs:
  GP16 → WS2812 Data (moved from GP28)

Encoder:
  GP20 → Switch
  GP21 → DT
  GP22 → CLK
```

## Schematic Reference

```
RP2350B Pico2
│
├─ GP26 ────── ADC0 (Key 0)
├─ GP27 ────── ADC1 (Key 1)
├─ GP28 ────── ADC2 (Key 2) ⚠️ Conflicts with WS2812!
├─ GP29 ────── ADC3 (Key 3)
├─ GP40 ────── ADC4 (Key 4)
├─ GP41 ────── ADC5 (Key 5)
├─ GP42 ────── ADC6 (Key 6)
├─ GP43 ────── ADC7 (Key 7)
│
├─ GP16 ────── WS2812 Data (recommended location)
│
├─ GP20 ────── Encoder SW (button)
├─ GP21 ────── Encoder DT
├─ GP22 ────── Encoder CLK
│
└─ USB  ────── HID Keyboard + CDC Serial
```
