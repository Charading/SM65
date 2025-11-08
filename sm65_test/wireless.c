#include "wireless.h"
#include "sm65_test.h"

/* The wireless implementation uses the Raspberry Pi Pico SDK (hardware/spi.h,
 * pico/stdlib.h). Those headers are only available when the Pico SDK is
 * enabled for the build. Guard the includes and implementation with
 * PICO_SDK so builds for other MCUs won't fail.
 */
#ifdef PICO_SDK
#include "hardware/spi.h"
#include "pico/stdlib.h"
#endif

#define PIN_MISO 16
#define PIN_CS   17
#define PIN_SCK  18
#define PIN_MOSI 19

void wireless_init(void) {
#ifdef PICO_SDK
    spi_init(spi0, 4000 * 1000); // 4 MHz
    gpio_set_function(PIN_MISO, GPIO_FUNC_SPI);
    gpio_set_function(PIN_SCK,  GPIO_FUNC_SPI);
    gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);
    gpio_init(PIN_CS);
    gpio_set_dir(PIN_CS, GPIO_OUT);
    gpio_put(PIN_CS, 1);
#else
    // Pico SDK not available; provide a no-op implementation so non-Pico
    // builds still link. If you intend to use wireless on another MCU,
    // implement the appropriate SPI hooks here.
#endif
}

/* Send any packet of bytes to the nRF */
void wireless_send(uint8_t *data, size_t len) {
#ifdef PICO_SDK
    gpio_put(PIN_CS, 0);
    spi_write_blocking(spi0, data, len);
    gpio_put(PIN_CS, 1);
#else
    // No-op for non-Pico builds
    (void)data;
    (void)len;
#endif
}

/* Receive bytes from the nRF.
 * Returns number of bytes read (up to max_len).
 * Call this periodically or from matrix_scan_kb() if you want to poll.
 */
size_t wireless_receive(uint8_t *buffer, size_t max_len) {
#ifdef PICO_SDK
    if (!buffer || max_len == 0) return 0;

    gpio_put(PIN_CS, 0);

    // Send dummy bytes (0xFF) to read back nRF data
    spi_read_blocking(spi0, 0xFF, buffer, max_len);

    gpio_put(PIN_CS, 1);
    return max_len;
#else
    // Not supported on non-Pico builds
    (void)buffer;
    (void)max_len;
    return 0;
#endif
}
