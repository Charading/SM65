#include <stdio.h>
#include "pico/stdlib.h"
#include "config.h"
#include "adc.h"
#include "encoder.h"
#include "usb.h"
#include "led.h"
#include "serial.h"

// HID keycodes for keys 0-7 (configured in config.h)
static const uint8_t number_keycodes[8] = {
    KEYCODE_0,
    KEYCODE_1,
    KEYCODE_2,
    KEYCODE_3,
    KEYCODE_4,
    KEYCODE_5,
    KEYCODE_6,
    KEYCODE_7
};

int main() {
    // Initialize USB HID and CDC FIRST (before anything else)
    usb_hid_init();
    serial_init();
    
    // Wait for USB enumeration
    sleep_ms(1000);
    
    serial_printf("RP2350B ADC Keyboard Initializing...\r\n");
    
    // Initialize peripherals
    adc_init_module();
    encoder_init();
    led_init();
    
    serial_printf("Calibrating ADC...\r\n");
    
    // Calibrate ADC (do this when no keys are pressed)
    // Visual feedback during calibration
    rgb_t calibration_color = {50, 50, 0}; // Yellow
    led_set_all(calibration_color);
    led_update();
    
    sleep_ms(500); // Give user time to release any keys
    adc_calibrate();
    
    led_clear();
    
    serial_printf("Calibration complete!\r\n");
    serial_printf("System ready.\r\n\r\n");
    
    // Track previous key states
    static uint8_t last_key_mask = 0;
    
    // Counter for periodic ADC value printing
    uint32_t print_counter = 0;
    const uint32_t print_interval = 100; // Print every 100 loops (~100ms)
    
    while (true) {
        // Process USB tasks
        usb_hid_task();
        serial_task();
        
        // Process ADC and get key states
        uint8_t key_mask = adc_process();
        
        // Handle key press/release events
        for (int i = 0; i < 8; i++) {
            bool current = (key_mask & (1 << i)) != 0;
            bool previous = (last_key_mask & (1 << i)) != 0;
            
            if (current && !previous) {
                // Key pressed
                usb_keyboard_press(number_keycodes[i]);
                serial_printf("Key %d pressed\r\n", i);
            } else if (!current && previous) {
                // Key released
                usb_keyboard_release(number_keycodes[i]);
                serial_printf("Key %d released\r\n", i);
            }
        }
        
        last_key_mask = key_mask;
        
        // Update LEDs based on key states
        led_update_keys(key_mask);
        
        // Process encoder
        encoder_event_t encoder_event = encoder_process();
        
        switch (encoder_event) {
            case ENCODER_EVENT_CW:
                usb_consumer_volume_up();
                serial_printf("Volume Up\r\n");
                break;
                
            case ENCODER_EVENT_CCW:
                usb_consumer_volume_down();
                serial_printf("Volume Down\r\n");
                break;
                
            case ENCODER_EVENT_PRESS:
                usb_consumer_mute();
                serial_printf("Mute Toggle\r\n");
                break;
                
            default:
                break;
        }
        
        // Periodically print ADC values
        if (++print_counter >= print_interval) {
            print_counter = 0;
            
            uint16_t adc_values[8];
            uint16_t adc_baseline[8];
            adc_get_values(adc_values);
            adc_get_baseline(adc_baseline);
            
            serial_print_adc_values(adc_values, adc_baseline);
        }
        
        // Small delay to prevent overwhelming the system
        sleep_ms(1);
    }
    
    return 0;
}
