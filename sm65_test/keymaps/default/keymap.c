// Copyright 2023 QMK
// SPDX-License-Identifier: GPL-2.0-or-later

#include QMK_KEYBOARD_H
#include "wireless.h"

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    /*
     * ┌───┐
     * │ A │
     * └───┘
     */
    [0] = LAYOUT_ortho_1x1(
        KC_A
    )
};

bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    if (record->event.pressed) {
        uint8_t packet[2] = { keycode & 0xFF, 1 }; // [key, pressed]
        wireless_send(packet, sizeof(packet));
    } else {
        uint8_t packet[2] = { keycode & 0xFF, 0 }; // [key, released]
        wireless_send(packet, sizeof(packet));
    }
    return true;
}
