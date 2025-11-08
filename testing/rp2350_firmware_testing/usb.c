#include "usb.h"
#include "tusb.h"
#include "pico/stdlib.h"
#include <string.h>

// HID Report IDs
#define REPORT_ID_KEYBOARD      1
#define REPORT_ID_CONSUMER      2

// Keyboard report structure
static struct {
    uint8_t modifiers;
    uint8_t reserved;
    uint8_t keys[6];
} keyboard_report = {0};

// Consumer control report
static uint16_t consumer_report = 0;

// HID report descriptor combining keyboard and consumer control
static const uint8_t hid_report_descriptor[] = {
    // Keyboard Report
    0x05, 0x01,        // Usage Page (Generic Desktop)
    0x09, 0x06,        // Usage (Keyboard)
    0xA1, 0x01,        // Collection (Application)
    0x85, REPORT_ID_KEYBOARD,  // Report ID (1)
    
    // Modifier keys
    0x05, 0x07,        //   Usage Page (Key Codes)
    0x19, 0xE0,        //   Usage Minimum (224)
    0x29, 0xE7,        //   Usage Maximum (231)
    0x15, 0x00,        //   Logical Minimum (0)
    0x25, 0x01,        //   Logical Maximum (1)
    0x75, 0x01,        //   Report Size (1)
    0x95, 0x08,        //   Report Count (8)
    0x81, 0x02,        //   Input (Data, Variable, Absolute)
    
    // Reserved byte
    0x95, 0x01,        //   Report Count (1)
    0x75, 0x08,        //   Report Size (8)
    0x81, 0x01,        //   Input (Constant)
    
    // Key array (6 keys)
    0x95, 0x06,        //   Report Count (6)
    0x75, 0x08,        //   Report Size (8)
    0x15, 0x00,        //   Logical Minimum (0)
    0x25, 0x65,        //   Logical Maximum (101)
    0x05, 0x07,        //   Usage Page (Key Codes)
    0x19, 0x00,        //   Usage Minimum (0)
    0x29, 0x65,        //   Usage Maximum (101)
    0x81, 0x00,        //   Input (Data, Array)
    0xC0,              // End Collection
    
    // Consumer Control Report
    0x05, 0x0C,        // Usage Page (Consumer)
    0x09, 0x01,        // Usage (Consumer Control)
    0xA1, 0x01,        // Collection (Application)
    0x85, REPORT_ID_CONSUMER,  // Report ID (2)
    0x15, 0x00,        //   Logical Minimum (0)
    0x26, 0xFF, 0x03,  //   Logical Maximum (1023)
    0x19, 0x00,        //   Usage Minimum (0)
    0x2A, 0xFF, 0x03,  //   Usage Maximum (1023)
    0x75, 0x10,        //   Report Size (16)
    0x95, 0x01,        //   Report Count (1)
    0x81, 0x00,        //   Input (Data, Array)
    0xC0,              // End Collection
};

// TinyUSB device callbacks
void tud_mount_cb(void) {
    // Called when device is mounted (configured)
}

void tud_umount_cb(void) {
    // Called when device is unmounted
}

void tud_suspend_cb(bool remote_wakeup_en) {
    (void) remote_wakeup_en;
}

void tud_resume_cb(void) {
}

// USB HID callbacks
void tud_hid_report_complete_cb(uint8_t instance, uint8_t const* report, uint16_t len) {
    (void) instance;
    (void) report;
    (void) len;
}

uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen) {
    (void) instance;
    (void) report_type;
    (void) reqlen;

    if (report_id == REPORT_ID_KEYBOARD) {
        memcpy(buffer, &keyboard_report, sizeof(keyboard_report));
        return sizeof(keyboard_report);
    } else if (report_id == REPORT_ID_CONSUMER) {
        memcpy(buffer, &consumer_report, sizeof(consumer_report));
        return sizeof(consumer_report);
    }
    
    return 0;
}

void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize) {
    (void) instance;
    (void) report_id;
    (void) report_type;
    (void) buffer;
    (void) bufsize;
}

// HID report descriptor length (needed for descriptor)
uint16_t tud_hid_descriptor_report_len(void) {
    return sizeof(hid_report_descriptor);
}

// TinyUSB descriptor callbacks
uint8_t const* tud_hid_descriptor_report_cb(uint8_t instance) {
    (void) instance;
    return hid_report_descriptor;
}

void usb_hid_init(void) {
    tusb_init();
}

bool usb_hid_ready(void) {
    return tud_hid_ready();
}

void usb_keyboard_press(uint8_t key) {
    // Add key to report if not already present
    for (int i = 0; i < 6; i++) {
        if (keyboard_report.keys[i] == key) {
            return; // Already pressed
        }
        if (keyboard_report.keys[i] == 0) {
            keyboard_report.keys[i] = key;
            break;
        }
    }
}

void usb_keyboard_release(uint8_t key) {
    // Remove key from report
    for (int i = 0; i < 6; i++) {
        if (keyboard_report.keys[i] == key) {
            keyboard_report.keys[i] = 0;
            // Shift remaining keys down
            for (int j = i; j < 5; j++) {
                keyboard_report.keys[j] = keyboard_report.keys[j + 1];
            }
            keyboard_report.keys[5] = 0;
            break;
        }
    }
}

void usb_keyboard_release_all(void) {
    memset(&keyboard_report, 0, sizeof(keyboard_report));
}

void usb_consumer_volume_up(void) {
    consumer_report = 0x00E9; // Volume Up
    if (tud_hid_ready()) {
        tud_hid_report(REPORT_ID_CONSUMER, &consumer_report, sizeof(consumer_report));
    }
    sleep_ms(10);
    consumer_report = 0;
    if (tud_hid_ready()) {
        tud_hid_report(REPORT_ID_CONSUMER, &consumer_report, sizeof(consumer_report));
    }
}

void usb_consumer_volume_down(void) {
    consumer_report = 0x00EA; // Volume Down
    if (tud_hid_ready()) {
        tud_hid_report(REPORT_ID_CONSUMER, &consumer_report, sizeof(consumer_report));
    }
    sleep_ms(10);
    consumer_report = 0;
    if (tud_hid_ready()) {
        tud_hid_report(REPORT_ID_CONSUMER, &consumer_report, sizeof(consumer_report));
    }
}

void usb_consumer_mute(void) {
    consumer_report = 0x00E2; // Mute
    if (tud_hid_ready()) {
        tud_hid_report(REPORT_ID_CONSUMER, &consumer_report, sizeof(consumer_report));
    }
    sleep_ms(10);
    consumer_report = 0;
    if (tud_hid_ready()) {
        tud_hid_report(REPORT_ID_CONSUMER, &consumer_report, sizeof(consumer_report));
    }
}

void usb_hid_task(void) {
    tud_task();
    
    // Send keyboard report if ready
    if (tud_hid_ready()) {
        tud_hid_report(REPORT_ID_KEYBOARD, &keyboard_report, sizeof(keyboard_report));
    }
}
