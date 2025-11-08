#include "serial.h"
#include "tusb.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

static char print_buffer[256];

// TinyUSB CDC callbacks
void tud_cdc_line_state_cb(uint8_t itf, bool dtr, bool rts) {
    (void) itf;
    (void) dtr;
    (void) rts;
}

void tud_cdc_rx_cb(uint8_t itf) {
    (void) itf;
}

void serial_init(void) {
    // CDC is initialized as part of TinyUSB init
    // Nothing specific needed here
}

bool serial_connected(void) {
    return tud_cdc_connected();
}

void serial_print_adc_values(uint16_t *values, uint16_t *baseline) {
    if (!tud_cdc_connected()) return;
    
    // Format: "ADC: CH0=1234(1200) CH1=2345(2300) ..."
    int pos = 0;
    pos += snprintf(print_buffer + pos, sizeof(print_buffer) - pos, "ADC: ");
    
    for (int i = 0; i < 8; i++) {
        pos += snprintf(print_buffer + pos, sizeof(print_buffer) - pos,
                       "CH%d=%4u(%4u) ", i, values[i], baseline[i]);
        if (pos >= sizeof(print_buffer) - 20) break;
    }
    
    pos += snprintf(print_buffer + pos, sizeof(print_buffer) - pos, "\r\n");
    
    tud_cdc_write(print_buffer, pos);
    tud_cdc_write_flush();
}

void serial_printf(const char *format, ...) {
    if (!tud_cdc_connected()) return;
    
    va_list args;
    va_start(args, format);
    int len = vsnprintf(print_buffer, sizeof(print_buffer), format, args);
    va_end(args);
    
    if (len > 0) {
        tud_cdc_write(print_buffer, len);
        tud_cdc_write_flush();
    }
}

void serial_task(void) {
    // Handle any incoming serial data if needed
    if (tud_cdc_available()) {
        uint8_t buf[64];
        uint32_t count = tud_cdc_read(buf, sizeof(buf));
        
        // Echo back for testing
        if (count > 0) {
            tud_cdc_write(buf, count);
            tud_cdc_write_flush();
        }
    }
}
