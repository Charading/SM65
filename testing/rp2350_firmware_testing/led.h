#ifndef LED_H
#define LED_H

#include <stdint.h>
#include <stdbool.h>

#define LED_PIN 28
#define LED_COUNT 8

// RGB color structure
typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} rgb_t;

/**
 * @brief Initialize WS2812 LED control
 * 
 * Sets up PIO for WS2812 control
 */
void led_init(void);

/**
 * @brief Set color of a single LED
 * 
 * @param index LED index (0-7)
 * @param color RGB color value
 */
void led_set_color(uint8_t index, rgb_t color);

/**
 * @brief Set color of all LEDs
 * 
 * @param color RGB color value
 */
void led_set_all(rgb_t color);

/**
 * @brief Update LED strip (send data to LEDs)
 * 
 * Must be called after setting colors to actually update the LEDs
 */
void led_update(void);

/**
 * @brief Clear all LEDs (turn off)
 */
void led_clear(void);

/**
 * @brief Set LED based on key state
 * 
 * @param key_mask Bitmask of pressed keys
 */
void led_update_keys(uint8_t key_mask);

/**
 * @brief Process VIA/SignalRGB commands (placeholder for future implementation)
 * 
 * @param data Command data
 * @param length Data length
 */
void led_process_via_command(uint8_t* data, uint16_t length);

#endif // LED_H
