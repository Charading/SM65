#ifndef ADC_H
#define ADC_H

#include <stdint.h>
#include <stdbool.h>

// ADC channel definitions for RP2350B
// The RP2350B has 8 ADC channels:
// ADC0 = GP26, ADC1 = GP27, ADC2 = GP28, ADC3 = GP29
// ADC4 = GP40, ADC5 = GP41, ADC6 = GP42, ADC7 = GP43

#ifndef NUM_ADC_CHANNELS
#define NUM_ADC_CHANNELS 8
#endif

#define ADC_DEVIATION_THRESHOLD 0.10f  // 10% deviation threshold

typedef struct {
    uint16_t baseline[NUM_ADC_CHANNELS];  // Calibrated baseline values
    bool key_pressed[NUM_ADC_CHANNELS];   // Current key state
    float threshold_low[NUM_ADC_CHANNELS]; // Lower threshold
    float threshold_high[NUM_ADC_CHANNELS]; // Upper threshold
} adc_state_t;

/**
 * @brief Initialize ADC subsystem
 * 
 * Initializes the ADC hardware and prepares for reading
 */
void adc_init_module(void);

/**
 * @brief Calibrate ADC baseline values
 * 
 * Reads all ADC channels multiple times and establishes baseline values
 * Should be called at startup when no keys are pressed
 */
void adc_calibrate(void);

/**
 * @brief Process ADC readings and detect key presses
 * 
 * Reads all ADC channels, compares against baseline with threshold,
 * and returns a bitmask of pressed keys
 * 
 * @return uint8_t Bitmask of pressed keys (bit 0 = key 0, bit 7 = key 7)
 */
uint8_t adc_process(void);

/**
 * @brief Get current ADC values for all channels
 * 
 * @param values Array of 8 uint16_t to store current ADC readings
 */
void adc_get_values(uint16_t *values);

/**
 * @brief Get baseline values for all channels
 * 
 * @param values Array of 8 uint16_t to store baseline readings
 */
void adc_get_baseline(uint16_t *values);

#endif // ADC_H
