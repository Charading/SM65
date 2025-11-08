// Minimal TinyUSB test - just enumerate as CDC device
#include "pico/stdlib.h"
#include "tusb.h"

int main() {
    // Initialize TinyUSB
    tusb_init();
    
    // Blink LED to show we're running
    const uint LED_PIN = 25;
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    
    while (1) {
        tud_task(); // TinyUSB device task
        
        // Blink LED
        gpio_put(LED_PIN, 1);
        sleep_ms(250);
        gpio_put(LED_PIN, 0);
        sleep_ms(250);
        
        // If connected, send a test message
        if (tud_cdc_connected()) {
            tud_cdc_write_str("Hello from RP2350!\r\n");
            tud_cdc_write_flush();
            sleep_ms(1000);
        }
    }
}
