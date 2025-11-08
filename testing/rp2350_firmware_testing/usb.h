#ifndef USB_H
#define USB_H

#include <stdint.h>
#include <stdbool.h>

// USB initialization and management
void usb_hid_init(void);
bool usb_hid_ready(void);

// Keyboard functions
void usb_keyboard_press(uint8_t key);
void usb_keyboard_release(uint8_t key);
void usb_keyboard_release_all(void);

// Consumer control (volume/mute) functions
void usb_consumer_volume_up(void);
void usb_consumer_volume_down(void);
void usb_consumer_mute(void);

// Task function - must be called regularly
void usb_hid_task(void);

#endif // USB_H
