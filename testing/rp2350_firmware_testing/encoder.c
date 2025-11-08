#include "encoder.h"
#include "hardware/gpio.h"
#include "pico/stdlib.h"

// Encoder state
static struct {
    uint8_t last_clk;
    uint8_t last_dt;
    bool button_pressed;
    bool button_last_state;
    uint32_t last_button_time;
} encoder_state;

// Debounce time in milliseconds
#define DEBOUNCE_MS 5

void encoder_init(void) {
    // Initialize CLK pin
    gpio_init(ENCODER_CLK_PIN);
    gpio_set_dir(ENCODER_CLK_PIN, GPIO_IN);
    gpio_pull_up(ENCODER_CLK_PIN);
    
    // Initialize DT pin
    gpio_init(ENCODER_DT_PIN);
    gpio_set_dir(ENCODER_DT_PIN, GPIO_IN);
    gpio_pull_up(ENCODER_DT_PIN);
    
    // Initialize SW (button) pin
    gpio_init(ENCODER_SW_PIN);
    gpio_set_dir(ENCODER_SW_PIN, GPIO_IN);
    gpio_pull_up(ENCODER_SW_PIN);
    
    // Read initial states
    encoder_state.last_clk = gpio_get(ENCODER_CLK_PIN);
    encoder_state.last_dt = gpio_get(ENCODER_DT_PIN);
    encoder_state.button_pressed = false;
    encoder_state.button_last_state = gpio_get(ENCODER_SW_PIN);
    encoder_state.last_button_time = 0;
}

encoder_event_t encoder_process(void) {
    encoder_event_t event = ENCODER_EVENT_NONE;
    
    // Read current states
    uint8_t clk = gpio_get(ENCODER_CLK_PIN);
    uint8_t dt = gpio_get(ENCODER_DT_PIN);
    
    // Detect rotation
    // Standard rotary encoder logic: CLK changes while DT stable indicates direction
    if (clk != encoder_state.last_clk) {
        if (clk == 0) { // CLK went low (falling edge)
            if (dt == 1) {
                // DT is high when CLK falls = clockwise
                event = ENCODER_EVENT_CW;
            } else {
                // DT is low when CLK falls = counter-clockwise
                event = ENCODER_EVENT_CCW;
            }
        }
    }
    
    encoder_state.last_clk = clk;
    encoder_state.last_dt = dt;
    
    // Check button state with debouncing
    uint8_t button_state = !gpio_get(ENCODER_SW_PIN); // Active low, so invert
    uint32_t now = to_ms_since_boot(get_absolute_time());
    
    if (button_state != encoder_state.button_last_state) {
        if ((now - encoder_state.last_button_time) > DEBOUNCE_MS) {
            encoder_state.button_last_state = button_state;
            encoder_state.last_button_time = now;
            
            if (button_state && !encoder_state.button_pressed) {
                // Button pressed
                encoder_state.button_pressed = true;
                if (event == ENCODER_EVENT_NONE) {
                    event = ENCODER_EVENT_PRESS;
                }
            } else if (!button_state && encoder_state.button_pressed) {
                // Button released
                encoder_state.button_pressed = false;
                if (event == ENCODER_EVENT_NONE) {
                    event = ENCODER_EVENT_RELEASE;
                }
            }
        }
    }
    
    return event;
}

bool encoder_is_pressed(void) {
    return encoder_state.button_pressed;
}
