#include "led.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "pico/stdlib.h"
#include <string.h>

// WS2812 timing (in nanoseconds at 800kHz)
// T0H: 350ns, T0L: 800ns
// T1H: 700ns, T1L: 600ns
// Reset: >50us low

static rgb_t led_buffer[LED_COUNT];
static PIO pio;
static uint sm;
static uint offset;

// PIO program for WS2812
// Based on the standard WS2812 PIO example from Raspberry Pi Pico examples
static const uint16_t ws2812_program_instructions[] = {
    //     .wrap_target
    0x6221, //  0: out    x, 1            side 0 [2]
    0x1223, //  1: jmp    !x, 3           side 1 [2]
    0x1400, //  2: jmp    0               side 1 [4]
    0xa442, //  3: nop                    side 0 [4]
    //     .wrap
};

static const struct pio_program ws2812_program = {
    .instructions = ws2812_program_instructions,
    .length = 4,
    .origin = -1,
};

static inline void put_pixel(uint32_t pixel_grb) {
    pio_sm_put_blocking(pio, sm, pixel_grb << 8u);
}

static inline uint32_t urgb_u32(uint8_t r, uint8_t g, uint8_t b) {
    return ((uint32_t)(g) << 16) | ((uint32_t)(r) << 8) | (uint32_t)(b);
}

void led_init(void) {
    // Use PIO0
    pio = pio0;
    
    // Try to claim a state machine
    sm = pio_claim_unused_sm(pio, true);
    
    // Load program
    offset = pio_add_program(pio, &ws2812_program);
    
    // Configure state machine
    pio_sm_config c = pio_get_default_sm_config();
    
    // Map the OUT instruction's side-set pin to the LED pin
    sm_config_set_sideset_pins(&c, LED_PIN);
    
    // Set pin direction to output
    pio_sm_set_consecutive_pindirs(pio, sm, LED_PIN, 1, true);
    
    // Configure shift register
    sm_config_set_out_shift(&c, false, true, 24); // Shift left, autopull at 24 bits
    
    // Set clock divider for 800kHz (approximately)
    // System clock / (cycles_per_bit * target_freq)
    float div = clock_get_hz(clk_sys) / (8.0f * 800000.0f);
    sm_config_set_clkdiv(&c, div);
    
    // Initialize GPIO for PIO
    pio_gpio_init(pio, LED_PIN);
    
    // Load config and jump to start
    pio_sm_init(pio, sm, offset, &c);
    
    // Enable state machine
    pio_sm_set_enabled(pio, sm, true);
    
    // Clear LED buffer
    memset(led_buffer, 0, sizeof(led_buffer));
    led_update();
}

void led_set_color(uint8_t index, rgb_t color) {
    if (index < LED_COUNT) {
        led_buffer[index] = color;
    }
}

void led_set_all(rgb_t color) {
    for (int i = 0; i < LED_COUNT; i++) {
        led_buffer[i] = color;
    }
}

void led_update(void) {
    for (int i = 0; i < LED_COUNT; i++) {
        put_pixel(urgb_u32(led_buffer[i].r, led_buffer[i].g, led_buffer[i].b));
    }
    sleep_us(100); // Reset time
}

void led_clear(void) {
    rgb_t black = {0, 0, 0};
    led_set_all(black);
    led_update();
}

void led_update_keys(uint8_t key_mask) {
    // Light up LEDs corresponding to pressed keys
    for (int i = 0; i < LED_COUNT; i++) {
        if (key_mask & (1 << i)) {
            // Key pressed - set to color (e.g., green)
            led_buffer[i].r = 0;
            led_buffer[i].g = 50;
            led_buffer[i].b = 0;
        } else {
            // Key released - dim or off
            led_buffer[i].r = 5;
            led_buffer[i].g = 0;
            led_buffer[i].b = 5;
        }
    }
    led_update();
}

void led_process_via_command(uint8_t* data, uint16_t length) {
    // Placeholder for VIA/SignalRGB protocol implementation
    // VIA uses raw HID reports for communication
    // SignalRGB has its own protocol
    
    // Basic structure for VIA RGB commands:
    // Command ID at data[0]
    // Subcommand at data[1] (if applicable)
    
    if (length < 2) return;
    
    uint8_t command = data[0];
    uint8_t subcommand = data[1];
    
    // Example VIA RGB commands (adjust based on actual VIA protocol)
    switch (command) {
        case 0x07: // RGB Matrix command (example)
            switch (subcommand) {
                case 0x00: // Set all LEDs
                    if (length >= 5) {
                        rgb_t color = {data[2], data[3], data[4]};
                        led_set_all(color);
                        led_update();
                    }
                    break;
                case 0x01: // Set single LED
                    if (length >= 6) {
                        uint8_t index = data[2];
                        rgb_t color = {data[3], data[4], data[5]};
                        led_set_color(index, color);
                        led_update();
                    }
                    break;
                default:
                    break;
            }
            break;
        default:
            break;
    }
    
    // Note: Full VIA and SignalRGB implementation requires:
    // 1. Raw HID endpoint in USB descriptors
    // 2. Proper protocol parsing
    // 3. EEPROM storage for settings
    // 4. RGB matrix effects library
}
