#include "wireless.h"

void keyboard_pre_init_kb(void) {
    wireless_init();
    keyboard_pre_init_user(); // don’t remove this
}

void matrix_scan_kb(void) {
    uint8_t rx[4];
    wireless_receive(rx, sizeof(rx));

    // Example: check for “message start” or simple command
    if (rx[0] == 0xAA) {
        // Handle whatever data you expect from the nRF
    }

    matrix_scan_user(); // keep default behavior
}