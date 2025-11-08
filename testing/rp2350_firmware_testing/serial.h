#ifndef SERIAL_H
#define SERIAL_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Initialize serial interface (USB CDC)
 */
void serial_init(void);

/**
 * @brief Check if serial is connected and ready
 * 
 * @return true if connected, false otherwise
 */
bool serial_connected(void);

/**
 * @brief Print ADC values to serial
 * 
 * @param values Array of ADC values
 * @param baseline Array of baseline values
 */
void serial_print_adc_values(uint16_t *values, uint16_t *baseline);

/**
 * @brief Print a formatted string to serial
 * 
 * @param format printf-style format string
 */
void serial_printf(const char *format, ...);

/**
 * @brief Process serial tasks (must be called regularly)
 */
void serial_task(void);

#endif // SERIAL_H
