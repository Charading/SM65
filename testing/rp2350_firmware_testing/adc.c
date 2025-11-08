#include "adc.h"
#include "hardware/adc.h"
#include "hardware/gpio.h"
#include "pico/stdlib.h"
#include <string.h>

// RP2350B has 8 ADC channels!
// ADC0 = GP26, ADC1 = GP27, ADC2 = GP28, ADC3 = GP29
// ADC4 = GP40, ADC5 = GP41, ADC6 = GP42, ADC7 = GP43

static adc_state_t adc_state;

// Map ADC channels to GPIO pins for RP2350B
static const uint8_t adc_gpio_map[8] = {26, 27, 28, 29, 40, 41, 42, 43};

void adc_init_module(void) {
    // Initialize ADC hardware
    adc_init();
    
    // Initialize all 8 GPIO pins for ADC on RP2350B
    for (int i = 0; i < NUM_ADC_CHANNELS; i++) {
        adc_gpio_init(adc_gpio_map[i]);
    }
    
    // Clear state
    memset(&adc_state, 0, sizeof(adc_state_t));
}

void adc_calibrate(void) {
    const int samples = 100;
    uint32_t accumulator[NUM_ADC_CHANNELS] = {0};
    
    // Take multiple samples and average for all 8 channels
    for (int sample = 0; sample < samples; sample++) {
        for (int ch = 0; ch < NUM_ADC_CHANNELS; ch++) {
            adc_select_input(ch);
            sleep_us(10); // Allow ADC to settle
            accumulator[ch] += adc_read();
        }
        sleep_ms(1);
    }
    
    // Calculate baseline and thresholds for all 8 channels
    for (int ch = 0; ch < NUM_ADC_CHANNELS; ch++) {
        adc_state.baseline[ch] = accumulator[ch] / samples;
        
        // Calculate threshold values (10% deviation)
        float baseline_float = (float)adc_state.baseline[ch];
        float deviation = baseline_float * ADC_DEVIATION_THRESHOLD;
        
        adc_state.threshold_low[ch] = baseline_float - deviation;
        adc_state.threshold_high[ch] = baseline_float + deviation;
        
        adc_state.key_pressed[ch] = false;
    }
}

uint8_t adc_process(void) {
    uint8_t key_mask = 0;
    uint16_t current_value;
    
    // Process all 8 ADC channels on RP2350B
    for (int ch = 0; ch < NUM_ADC_CHANNELS; ch++) {
        adc_select_input(ch);
        sleep_us(10); // Allow ADC to settle
        current_value = adc_read();
        
        // Check if value has deviated by more than threshold
        bool is_deviated = (current_value < adc_state.threshold_low[ch]) || 
                          (current_value > adc_state.threshold_high[ch]);
        
        // Update key state with basic debouncing
        if (is_deviated && !adc_state.key_pressed[ch]) {
            adc_state.key_pressed[ch] = true;
            key_mask |= (1 << ch);
        } else if (!is_deviated && adc_state.key_pressed[ch]) {
            adc_state.key_pressed[ch] = false;
        }
        
        // Add current state to mask (for key hold)
        if (adc_state.key_pressed[ch]) {
            key_mask |= (1 << ch);
        }
    }
    
    return key_mask;
}

void adc_get_values(uint16_t *values) {
    // Read all 8 ADC channels on RP2350B
    for (int ch = 0; ch < NUM_ADC_CHANNELS; ch++) {
        adc_select_input(ch);
        sleep_us(10);
        values[ch] = adc_read();
    }
}

void adc_get_baseline(uint16_t *values) {
    for (int ch = 0; ch < NUM_ADC_CHANNELS; ch++) {
        values[ch] = adc_state.baseline[ch];
    }
}
