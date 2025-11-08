#ifndef ENCODER_H
#define ENCODER_H

#include <stdint.h>
#include <stdbool.h>

// Encoder pin definitions
#define ENCODER_CLK_PIN 22
#define ENCODER_DT_PIN  21
#define ENCODER_SW_PIN  20

// Encoder events
typedef enum {
    ENCODER_EVENT_NONE = 0,
    ENCODER_EVENT_CW,        // Clockwise rotation (volume up)
    ENCODER_EVENT_CCW,       // Counter-clockwise rotation (volume down)
    ENCODER_EVENT_PRESS,     // Button press (mute)
    ENCODER_EVENT_RELEASE    // Button release
} encoder_event_t;

/**
 * @brief Initialize the rotary encoder
 * 
 * Sets up GPIO pins and interrupts for the encoder
 */
void encoder_init(void);

/**
 * @brief Process encoder state and return events
 * 
 * Should be called regularly from main loop
 * 
 * @return encoder_event_t The detected encoder event
 */
encoder_event_t encoder_process(void);

/**
 * @brief Check if encoder button is currently pressed
 * 
 * @return true if button is pressed, false otherwise
 */
bool encoder_is_pressed(void);

#endif // ENCODER_H
