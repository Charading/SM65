#include "tusb.h"
#include "pico/unique_id.h"
#include <string.h>
#include <stdio.h>

// Device descriptor
tusb_desc_device_t const desc_device = {
    .bLength            = sizeof(tusb_desc_device_t),
    .bDescriptorType    = TUSB_DESC_DEVICE,
    .bcdUSB             = 0x0200,
    .bDeviceClass       = 0x00,
    .bDeviceSubClass    = 0x00,
    .bDeviceProtocol    = 0x00,
    .bMaxPacketSize0    = CFG_TUD_ENDPOINT0_SIZE,
    .idVendor           = 0x2E8A,  // Raspberry Pi
    .idProduct          = 0x000A,  // Raspberry Pi Pico SDK CDC
    .bcdDevice          = 0x0100,
    .iManufacturer      = 0x01,
    .iProduct           = 0x02,
    .iSerialNumber      = 0x03,
    .bNumConfigurations = 0x01
};

uint8_t const* tud_descriptor_device_cb(void) {
    return (uint8_t const*) &desc_device;
}

// HID Report Descriptor defined in usb.c
extern uint8_t const* tud_hid_descriptor_report_cb(uint8_t instance);

enum {
    ITF_NUM_HID,
    ITF_NUM_CDC_0,
    ITF_NUM_CDC_0_DATA,
    ITF_NUM_TOTAL
};

// Calculate the actual HID report descriptor length
// Keyboard report: 53 bytes + Consumer report: 18 bytes = 71 bytes total
#define HID_REPORT_DESC_LEN 71

#define CONFIG_TOTAL_LEN    (TUD_CONFIG_DESC_LEN + TUD_HID_DESC_LEN + TUD_CDC_DESC_LEN)

#define EPNUM_HID           0x81
#define EPNUM_CDC_NOTIF     0x82
#define EPNUM_CDC_OUT       0x03
#define EPNUM_CDC_IN        0x83

uint8_t const desc_configuration[] = {
    // Config number, interface count, string index, total length, attribute, power in mA
    TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, CONFIG_TOTAL_LEN, 0x00, 500),

    // Interface number, string index, protocol, report descriptor len, EP In address, size & polling interval
    TUD_HID_DESCRIPTOR(ITF_NUM_HID, 0, HID_ITF_PROTOCOL_NONE, HID_REPORT_DESC_LEN, EPNUM_HID, CFG_TUD_HID_EP_BUFSIZE, 1),

    // Interface number, string index, EP notification address and size, EP data address (out, in) and size.
    TUD_CDC_DESCRIPTOR(ITF_NUM_CDC_0, 4, EPNUM_CDC_NOTIF, 8, EPNUM_CDC_OUT, EPNUM_CDC_IN, 64),
};

uint8_t const* tud_descriptor_configuration_cb(uint8_t index) {
    (void) index;
    return desc_configuration;
}

// String descriptors
char const* string_desc_arr[] = {
    (const char[]) { 0x09, 0x04 }, // 0: Language (English)
    "Charading",                    // 1: Manufacturer
    "RP2350 ADC Keyboard",          // 2: Product
    NULL,                           // 3: Serial (will be set dynamically)
    "RP2350 CDC Serial",            // 4: CDC Interface
};

static char serial_number_str[PICO_UNIQUE_BOARD_ID_SIZE_BYTES * 2 + 1];

static uint16_t _desc_str[32];

uint16_t const* tud_descriptor_string_cb(uint8_t index, uint16_t langid) {
    (void) langid;

    uint8_t chr_count;

    if (index == 0) {
        memcpy(&_desc_str[1], string_desc_arr[0], 2);
        chr_count = 1;
    } else if (index == 3) {
        // Generate serial number from unique ID
        pico_unique_board_id_t id;
        pico_get_unique_board_id(&id);
        
        for (int i = 0; i < PICO_UNIQUE_BOARD_ID_SIZE_BYTES; i++) {
            sprintf(&serial_number_str[i * 2], "%02X", id.id[i]);
        }
        
        chr_count = strlen(serial_number_str);
        for (uint8_t i = 0; i < chr_count; i++) {
            _desc_str[1 + i] = serial_number_str[i];
        }
    } else {
        if (index >= sizeof(string_desc_arr) / sizeof(string_desc_arr[0])) return NULL;

        const char* str = string_desc_arr[index];
        chr_count = strlen(str);
        
        // Convert ASCII to UTF-16
        for (uint8_t i = 0; i < chr_count; i++) {
            _desc_str[1 + i] = str[i];
        }
    }

    // Header: type (1 byte), length (1 byte)
    _desc_str[0] = (TUSB_DESC_STRING << 8) | (2 * chr_count + 2);

    return _desc_str;
}
