// mcp3208_demo.c
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/gpio.h"
#include <stdbool.h>
#include <string.h>

#define SPI_PORT spi0
#define PIN_SCK  18   // GP18
#define PIN_MOSI 19   // GP19
#define PIN_MISO 16   // GP16
#define PIN_CS   17   // GP17

// HC4067 mux select pins (assumed mapping: GP10 = S0, GP11 = S1, GP12 = S2, GP13 = S3)
#define PIN_MUX_S0 10
#define PIN_MUX_S1 11
#define PIN_MUX_S2 12
#define PIN_MUX_S3 13

// Set this to your actual Vref wiring (e.g., 3.300 or 2.500)
#define VREF_VOLTS 3.300f

// ------------------ Configuration (easy to change / make a submodule) ------------------
// Number of HC4067 mux chips you have (one per MCP3208 ADC channel)
#define MUX_COUNT 5

// Map each mux index [0..MUX_COUNT-1] to the MCP3208 ADC channel that reads it.
// Change these values to match your wiring. Example: mux 0 -> ADC0, mux1 -> ADC3, ...
static const uint8_t mux_to_adc[MUX_COUNT] = { 0, 3, 4, 6, 7 };

// Threshold below which readings are considered floating/unconnected and will NOT be printed
#define MUX_PRINT_THRESHOLD 200u
// Settle time after changing mux select (microseconds). Lower to speed up scans but
// don't set to 0 if your wiring needs time to settle.
#define MUX_SETTLE_US 200u
// Delay between full scans in milliseconds (lower = faster updates)
#define SCAN_DELAY_MS 80u
// ---------------------------------------------------------------------------------------
static uint16_t mcp3208_read(uint8_t ch) {
    // ch: 0..7
    // Transaction: 3 bytes
    //  - Byte0: 0b00000110 | (ch >> 2)   (Start=1, SGL/DIFF=1, D2)
    //  - Byte1: (ch & 0x3) << 6          (D1 D0 then 6 zeros)
    //  - Byte2: 0x00
    // Response: 12-bit value across rx1[3:0] and rx2[7:0]
    uint8_t tx[3];
    uint8_t rx[3];

    tx[0] = 0x06 | ((ch & 0x07) >> 2);
    tx[1] = (uint8_t)((ch & 0x03) << 6);
    tx[2] = 0x00;

    gpio_put(PIN_CS, 0);
    spi_write_read_blocking(SPI_PORT, tx, rx, 3);
    gpio_put(PIN_CS, 1);

    uint16_t value = ((rx[1] & 0x0F) << 8) | rx[2]; // 12 bits
    return value;
}

// Set the 4-bit select value on the HC4067 (S0..S3)
static inline void mux_set(uint8_t sel) {
    gpio_put(PIN_MUX_S0, sel & 0x1);
    gpio_put(PIN_MUX_S1, (sel >> 1) & 0x1);
    gpio_put(PIN_MUX_S2, (sel >> 2) & 0x1);
    gpio_put(PIN_MUX_S3, (sel >> 3) & 0x1);
}

// Print the current mux output states (reads back the gpio pins so you can verify actual outputs)
// (removed debug print of select bits to keep output compact)

int main() {
    stdio_init_all();

    // SPI setup
    spi_init(SPI_PORT, 1000 * 1000); // 1 MHz
    spi_set_format(SPI_PORT, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);

    gpio_set_function(PIN_SCK,  GPIO_FUNC_SPI);
    gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);
    gpio_set_function(PIN_MISO, GPIO_FUNC_SPI);

    // CS pin
    gpio_init(PIN_CS);
    gpio_set_dir(PIN_CS, GPIO_OUT);
    gpio_put(PIN_CS, 1);

    // Configure mux select pins as outputs
    gpio_init(PIN_MUX_S0);
    gpio_set_dir(PIN_MUX_S0, GPIO_OUT);
    gpio_put(PIN_MUX_S0, 0);

    gpio_init(PIN_MUX_S1);
    gpio_set_dir(PIN_MUX_S1, GPIO_OUT);
    gpio_put(PIN_MUX_S1, 0);

    gpio_init(PIN_MUX_S2);
    gpio_set_dir(PIN_MUX_S2, GPIO_OUT);
    gpio_put(PIN_MUX_S2, 0);

    gpio_init(PIN_MUX_S3);
    gpio_set_dir(PIN_MUX_S3, GPIO_OUT);
    gpio_put(PIN_MUX_S3, 0);

    // Give USB time to connect
    sleep_ms(1000);

    while (true) {
        // Build one output block in a buffer and print it in a single call to avoid line-by-line prints
        // This reduces interleaving and sends the whole scan as one packet to the serial monitor.
        char outbuf[2048];
        size_t off = 0;
        size_t left = sizeof(outbuf);

        for (uint8_t mux_idx = 0; mux_idx < MUX_COUNT; mux_idx++) {
            uint8_t adc_ch = mux_to_adc[mux_idx];

            // append header for this mux
            int n = snprintf(outbuf + off, left, "MUX %u", (unsigned)(mux_idx + 1));
            if (n < 0) n = 0;
            if ((size_t)n >= left) { off = sizeof(outbuf) - 1; left = 0; break; }
            off += (size_t)n; left -= (size_t)n;

            bool any_printed = false;
            for (uint8_t sel = 0; sel < 16; sel++) {
                mux_set(sel);
                // small settle time after switching the mux select lines
                sleep_us(MUX_SETTLE_US);
                uint16_t raw = mcp3208_read(adc_ch);
                if (raw >= MUX_PRINT_THRESHOLD && left > 0) {
                    n = snprintf(outbuf + off, left, " | CH%u: %u", (unsigned)(sel + 1), (unsigned)raw);
                    if (n < 0) n = 0;
                    if ((size_t)n >= left) { off = sizeof(outbuf) - 1; left = 0; break; }
                    off += (size_t)n; left -= (size_t)n;
                    any_printed = true;
                }
            }

            if (!any_printed && left > 0) {
                int m = snprintf(outbuf + off, left, " | (no readings >= %u)", (unsigned)MUX_PRINT_THRESHOLD);
                if (m < 0) m = 0;
                if ((size_t)m >= left) { off = sizeof(outbuf) - 1; left = 0; break; }
                off += (size_t)m; left -= (size_t)m;
            }

            if (left > 0) {
                int e = snprintf(outbuf + off, left, "\n");
                if (e < 0) e = 0;
                if ((size_t)e >= left) { off = sizeof(outbuf) - 1; left = 0; break; }
                off += (size_t)e; left -= (size_t)e;
            }
        }

        // append a separator line and print the entire block in a single call
        if (left > 0) {
            int s = snprintf(outbuf + off, left, "-----------\n");
            if (s > 0 && (size_t)s < left) { off += (size_t)s; left -= (size_t)s; }
        }
        if (off > 0) {
            // Ensure buffer is null-terminated
            outbuf[off < sizeof(outbuf) ? off : (sizeof(outbuf) - 1)] = '\0';
            printf("%s", outbuf);
        }
        // small pause between full scans (tunable)
        sleep_ms(SCAN_DELAY_MS);

        // pause between full scans
        sleep_ms(50);
    }
}
