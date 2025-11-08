#ifndef CONFIG_H
#define CONFIG_H

// ============================================================================
// KEYBOARD CONFIGURATION
// ============================================================================

// ADC Configuration (RP2350B has 8 ADC channels!)
#define ADC_NUM_CHANNELS        8
#define ADC_CALIBRATION_SAMPLES 100
#define ADC_DEVIATION_PERCENT   10      // 10% deviation triggers key press

// ADC GPIO Pins (RP2350B has ADC0-7)
#define ADC_GPIO_0              26      // ADC0
#define ADC_GPIO_1              27      // ADC1
#define ADC_GPIO_2              28      // ADC2
#define ADC_GPIO_3              29      // ADC3
#define ADC_GPIO_4              40      // ADC4
#define ADC_GPIO_5              41      // ADC5
#define ADC_GPIO_6              42      // ADC6
#define ADC_GPIO_7              43      // ADC7

// Rotary Encoder Pins
#define ENCODER_PIN_CLK         22
#define ENCODER_PIN_DT          21
#define ENCODER_PIN_SW          20

// WS2812 LED Configuration
// Note: GP28 conflicts with ADC2! Recommended to use GP16 instead
#define LED_GPIO_PIN            28      // Change to 16 to avoid ADC2 conflict
#define LED_NUM_LEDS            8
#define LED_BRIGHTNESS_MAX      50      // 0-255, lower = less power draw

// LED Colors (RGB values 0-255)
#define LED_COLOR_KEY_PRESSED_R   0
#define LED_COLOR_KEY_PRESSED_G   50
#define LED_COLOR_KEY_PRESSED_B   0

#define LED_COLOR_KEY_IDLE_R      5
#define LED_COLOR_KEY_IDLE_G      0
#define LED_COLOR_KEY_IDLE_B      5

#define LED_COLOR_CALIBRATING_R   50
#define LED_COLOR_CALIBRATING_G   50
#define LED_COLOR_CALIBRATING_B   0

// ============================================================================
// USB CONFIGURATION
// ============================================================================

#define USB_VID                 0xFEED  // Vendor ID (change for production)
#define USB_PID                 0x0350  // Product ID
#define USB_MANUFACTURER        "Charading"
#define USB_PRODUCT             "RP2350 ADC Keyboard"

// ============================================================================
// SERIAL CONFIGURATION
// ============================================================================

#define SERIAL_ADC_PRINT_INTERVAL_MS  100  // How often to print ADC values

// ============================================================================
// TIMING CONFIGURATION
// ============================================================================

#define MAIN_LOOP_DELAY_MS      1       // Main loop iteration delay
#define CALIBRATION_DELAY_MS    500     // Delay before calibration starts
#define ENCODER_DEBOUNCE_MS     5       // Encoder button debounce time

// ============================================================================
// FEATURE ENABLES
// ============================================================================

#define ENABLE_SERIAL_OUTPUT    1       // Enable USB CDC serial output
#define ENABLE_LED_FEEDBACK     1       // Enable LED visual feedback
#define ENABLE_ENCODER          1       // Enable rotary encoder
#define ENABLE_VIA_SUPPORT      0       // VIA support (not fully implemented)
#define ENABLE_SIGNALRGB        0       // SignalRGB support (not fully implemented)

// ============================================================================
// KEY MAPPINGS
// ============================================================================

// USB HID keycodes for keys 0-7
// Reference: https://www.usb.org/sites/default/files/documents/hut1_12v2.pdf
#define KEYCODE_0               0x27    // Keyboard 0
#define KEYCODE_1               0x1E    // Keyboard 1
#define KEYCODE_2               0x1F    // Keyboard 2
#define KEYCODE_3               0x20    // Keyboard 3
#define KEYCODE_4               0x21    // Keyboard 4
#define KEYCODE_5               0x22    // Keyboard 5
#define KEYCODE_6               0x23    // Keyboard 6
#define KEYCODE_7               0x24    // Keyboard 7

// Alternative keycode sets (comment/uncomment as needed)
// Letters A-H
// #define KEYCODE_0               0x04    // A
// #define KEYCODE_1               0x05    // B
// #define KEYCODE_2               0x06    // C
// #define KEYCODE_3               0x07    // D
// #define KEYCODE_4               0x08    // E
// #define KEYCODE_5               0x09    // F
// #define KEYCODE_6               0x0A    // G
// #define KEYCODE_7               0x0B    // H

// Function keys F1-F8
// #define KEYCODE_0               0x3A    // F1
// #define KEYCODE_1               0x3B    // F2
// #define KEYCODE_2               0x3C    // F3
// #define KEYCODE_3               0x3D    // F4
// #define KEYCODE_4               0x3E    // F5
// #define KEYCODE_5               0x3F    // F6
// #define KEYCODE_6               0x40    // F7
// #define KEYCODE_7               0x41    // F8

#endif // CONFIG_H
