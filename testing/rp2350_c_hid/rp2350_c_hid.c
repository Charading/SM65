#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/adc.h"
#include "bsp/board.h"
#include "tusb.h"
#include "config.h"

// GPIO pin for button input
#define BUTTON_PIN 30

// ADC configuration
#define ADC_VREF 3.3f
#define ADC_RESOLUTION 4096
#define NUM_MUXES 5
#define CHANNELS_PER_MUX 16
#define TOTAL_CHANNELS (NUM_MUXES * CHANNELS_PER_MUX)

// Mux control pins
const uint mux_select_pins[] = {MUX_S0, MUX_S1, MUX_S2, MUX_S3};
const uint mux_analog_pins[] = {MUX1_PIN, MUX2_PIN, MUX3_PIN, MUX4_PIN, MUX5_PIN};
// For RP2350B: GP26-29 = ADC0-3, GP40-47 = ADC4-11
// GP40=ADC4, GP41=ADC5, GP42=ADC6, GP43=ADC7, GP44=ADC8
const uint mux_adc_inputs[] = {4, 5, 6, 7, 8}; // ADC input numbers for GP40-44

// Initialize mux control pins
void init_mux_pins() {
    printf("Initializing mux system...\n");
    
    // Initialize select pins as outputs
    for (int i = 0; i < 4; i++) {
        gpio_init(mux_select_pins[i]);
        gpio_set_dir(mux_select_pins[i], GPIO_OUT);
        gpio_put(mux_select_pins[i], 0);
        printf("  S%d -> GP%d\n", i, mux_select_pins[i]);
    }
    
    // Initialize ADC
    printf("Initializing ADC...\n");
    adc_init();
    
    // Initialize ADC inputs for all mux analog pins
    printf("Initializing ADC GPIO pins:\n");
    for (int i = 0; i < NUM_MUXES; i++) {
        adc_gpio_init(mux_analog_pins[i]);
        printf("  MUX%d -> GP%d (ADC%d)\n", i+1, mux_analog_pins[i], mux_adc_inputs[i]);
    }
    printf("Mux initialization complete!\n\n");
}

// Set mux channel (0-15)
void set_mux_channel(uint8_t channel) {
    for (int i = 0; i < 4; i++) {
        gpio_put(mux_select_pins[i], (channel >> i) & 1);
    }
    // Small delay for mux settling. Increase to allow the analog line to charge
    // through the source impedance of the sensor + mux. 10us is often too
    // small for higher impedance sensors; use 200us here.
    sleep_us(200);
}

// Read ADC value from specific mux and channel
uint16_t read_mux_adc(uint8_t mux_index, uint8_t channel) {
    if (mux_index >= NUM_MUXES || channel >= CHANNELS_PER_MUX) {
        return 0;
    }
    
    // Set the mux channel
    set_mux_channel(channel);
    
    // Select the ADC input for this mux
    adc_select_input(mux_adc_inputs[mux_index]);
    
    // Trick: discard first sample after switching (sample & hold needs to settle),
    // then take a few samples and average. This helps when sensors are higher
    // impedance or when the mux+traces need time to settle.
    // Note: keep sample_count small to avoid slowing overall scan too much.
    const int sample_count = 3;
    // Short extra delay to let ADC sample capacitor settle to the new voltage
    sleep_us(50);
    // discard 1st read
    (void)adc_read();
    uint32_t sum = 0;
    for (int i = 0; i < sample_count; i++) {
        sum += adc_read();
        sleep_us(20);
    }
    return (uint16_t)(sum / sample_count);
}

// Convert ADC reading to voltage
float adc_to_voltage(uint16_t adc_value) {
    return (float)adc_value * ADC_VREF / ADC_RESOLUTION;
}

// Print all ADC values as one big block (one line per mux, raw values only)
void print_all_adc_values() {
    printf("\n=== ADC BLOCK START ===\n");

    for (int mux = 0; mux < NUM_MUXES; mux++) {
        // print raw ADC values for this mux, separated by spaces
        for (int channel = 0; channel < CHANNELS_PER_MUX; channel++) {
            uint16_t adc_raw = read_mux_adc(mux, channel);
            // Print zero-padded 4-digit raw value
            if (channel == 0) {
                printf("%04d", adc_raw);
            } else {
                printf(" %04d", adc_raw);
            }
        }
        // newline between mux rows
        printf("\n");
    }

    printf("=== ADC BLOCK END ===\n\n");
}

// HID report descriptor for keyboard
static const uint8_t desc_hid_report[] = {
    TUD_HID_REPORT_DESC_KEYBOARD()
};

// USB Device Descriptor
static const tusb_desc_device_t desc_device = {
    .bLength            = sizeof(tusb_desc_device_t),
    .bDescriptorType    = TUSB_DESC_DEVICE,
    .bcdUSB             = 0x0200,
    .bDeviceClass       = 0x00,
    .bDeviceSubClass    = 0x00,
    .bDeviceProtocol    = 0x00,
    .bMaxPacketSize0    = CFG_TUD_ENDPOINT0_SIZE,
    .idVendor           = 0xCafe,
    .idProduct          = 0x4001,
    .bcdDevice          = 0x0100,
    .iManufacturer      = 0x01,
    .iProduct           = 0x02,
    .iSerialNumber      = 0x03,
    .bNumConfigurations = 0x01
};

// Configuration Descriptor
enum {
    ITF_NUM_CDC = 0,
    ITF_NUM_CDC_DATA,
    ITF_NUM_HID,
    ITF_NUM_TOTAL
};

#define CONFIG_TOTAL_LEN  (TUD_CONFIG_DESC_LEN + TUD_CDC_DESC_LEN + TUD_HID_DESC_LEN)

static const uint8_t desc_configuration[] = {
    // Config number, interface count, string index, total length, attribute, power in mA
    TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, CONFIG_TOTAL_LEN, TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 100),

    // CDC (stdio) Interface
    // args: itfnum, stridx, ep_notif, ep_notif_size, ep_out, ep_in, epsize
    TUD_CDC_DESCRIPTOR(ITF_NUM_CDC, 4, 0x82, 8, 0x01, 0x81, CFG_TUD_CDC_EP_BUFSIZE),

    // HID Keyboard Interface
    TUD_HID_DESCRIPTOR(ITF_NUM_HID, 0, HID_ITF_PROTOCOL_KEYBOARD, sizeof(desc_hid_report), 0x83, CFG_TUD_HID_EP_BUFSIZE, 10)
};

// String Descriptors
static const char* string_desc_arr[] = {
    (const char[]) { 0x09, 0x04 }, // 0: is supported language is English (0x0409)
    "RP2350",                      // 1: Manufacturer
    "RP2350 HID Keyboard",         // 2: Product
    "123456",                      // 3: Serials, should use chip ID
    "CDC Serial"                   // 4: CDC interface string
};

static uint16_t _desc_str[32];

// Device callbacks
uint8_t const * tud_descriptor_device_cb(void) {
    return (uint8_t const *) &desc_device;
}

uint8_t const * tud_descriptor_configuration_cb(uint8_t index) {
    (void) index;
    return desc_configuration;
}

uint16_t const* tud_descriptor_string_cb(uint8_t index, uint16_t langid) {
    (void) langid;
    
    uint8_t chr_count;
    
    if (index == 0) {
        memcpy(&_desc_str[1], string_desc_arr[0], 2);
        chr_count = 1;
    } else {
        if (!(index < sizeof(string_desc_arr)/sizeof(string_desc_arr[0]))) return NULL;
        
        const char* str = string_desc_arr[index];
        chr_count = strlen(str);
        if (chr_count > 31) chr_count = 31;
        
        for(uint8_t i=0; i<chr_count; i++) {
            _desc_str[1+i] = str[i];
        }
    }
    
    _desc_str[0] = (TUSB_DESC_STRING << 8 ) | (2*chr_count + 2);
    
    return _desc_str;
}

// HID Report Descriptor
uint8_t const * tud_hid_descriptor_report_cb(uint8_t instance) {
    (void) instance;
    return desc_hid_report;
}

// HID callbacks
uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen) {
    (void) instance;
    (void) report_id;
    (void) report_type;
    (void) buffer;
    (void) reqlen;
    
    return 0;
}

void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize) {
    (void) instance;
    (void) report_id;
    (void) report_type;
    (void) buffer;
    (void) bufsize;
}

// Send keyboard report - returns true if sent successfully
bool send_hid_report(uint8_t modifier, uint8_t keycode) {
    if (tud_suspended() && (modifier || keycode)) {
        tud_remote_wakeup();
        return false; // Can't send while suspended
    }
    
    if (tud_hid_ready()) {
        uint8_t keycode_arr[6] = {0};
        
        if (keycode) {
            keycode_arr[0] = keycode;
        }
        
        return tud_hid_keyboard_report(0, modifier, keycode_arr);
    }
    
    return false; // HID not ready
}

// Main function
int main() {
    // Basic board and USB initialization
    board_init();
    tusb_init();

    // Initialize stdio to use USB CDC (enabled in CMake) so printf() goes over USB serial
    stdio_init_all();

    // Small delay to let USB enumerator/host settle
    sleep_ms(100);

    // Wait a short time for the host to open the USB CDC serial port
    // This prevents startup messages from being missed; timeout after 5s.
    {
        const uint32_t cdc_wait_ms = 5000;
        uint32_t wait_start = board_millis();
        printf("Waiting up to %lu ms for USB CDC connection...\n", cdc_wait_ms);
        while (!tud_cdc_connected() && (board_millis() - wait_start) < cdc_wait_ms) {
            tud_task(); // keep USB stack serviced
            sleep_ms(10);
        }
        if (tud_cdc_connected()) {
            printf("USB CDC connected, proceeding.\n");
        } else {
            printf("USB CDC not connected after %lu ms, continuing without host.\n", cdc_wait_ms);
        }
    }

    // Initialize GPIO pin for button input
    gpio_init(BUTTON_PIN);
    gpio_set_dir(BUTTON_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_PIN); // Enable pull-up resistor
    
    // Initialize mux and ADC system
    init_mux_pins();
    
    printf("RP2350B USB HID Keyboard with ADC Mux Scanner\n");
    printf("Device will enumerate as a keyboard\n");
    printf("Press GP30 to ground to send 'E' key\n");
    printf("5x HC4067 Muxes configured:\n");
    printf("  MUX1: GP%d, MUX2: GP%d, MUX3: GP%d, MUX4: GP%d, MUX5: GP%d\n", 
           MUX1_PIN, MUX2_PIN, MUX3_PIN, MUX4_PIN, MUX5_PIN);
    printf("  Select pins: S0=GP%d, S1=GP%d, S2=GP%d, S3=GP%d\n", 
           MUX_S0, MUX_S1, MUX_S2, MUX_S3);
    printf("  Total channels: %d (16 per mux)\n\n", TOTAL_CHANNELS);
    
    uint32_t blink_interval_ms = 1000;
    uint32_t start_ms = 0;
    uint32_t adc_scan_ms = 0;
    const uint32_t adc_scan_interval = 100; // Scan ADCs every 100 ms
    bool led_state = false;
    bool button_pressed = false; // Flag to track if we've already sent a key for this press
    
    while (1) {
        tud_task(); // TinyUSB device task
        
        // Blink LED to show activity
        uint32_t current_ms = board_millis();
        if (current_ms - start_ms >= blink_interval_ms) {
            start_ms = current_ms;
            board_led_write(led_state);
            led_state = !led_state;
        }
        
        // (No heartbeat messages by request) -- only USB CDC/stdout output occurs when needed.
        
        // Periodic ADC scanning
        if (current_ms - adc_scan_ms >= adc_scan_interval) {
            adc_scan_ms = current_ms;
            print_all_adc_values();
        }
        
        // Check if device is mounted (enumerated)
        static bool enumerated_message_sent = false;
        if (tud_mounted()) {
            if (blink_interval_ms != 250) {
                blink_interval_ms = 250;
                if (!enumerated_message_sent) {
                    printf("USB HID Keyboard enumerated successfully!\n");
                    printf("Ready to send 'E' when GP30 is pressed to ground\n");
                    enumerated_message_sent = true;
                }
            }
            
            // Simple button handling - just check current state
            bool current_button = !gpio_get(BUTTON_PIN); // Invert because active low
            
            // Only send key if button is pressed AND we haven't sent one for this press
            if (current_button && !button_pressed && tud_hid_ready()) {
                printf("Button detected low - sending 'E' key\n");
                
                // Send key press
                uint8_t keycode_arr[6] = {0};
                keycode_arr[0] = HID_KEY_E;
                
                if (tud_hid_keyboard_report(0, 0, keycode_arr)) {
                    printf("Key press sent\n");
                    button_pressed = true; // Prevent repeat until button is released
                    
                    // Schedule key release for next loop iteration
                    sleep_ms(50);
                } else {
                    printf("Failed to send key press\n");
                }
            }
            
            // Send key release if we've sent a press and HID is ready
            static bool need_key_release = false;
            if (button_pressed && !need_key_release && tud_hid_ready()) {
                need_key_release = true;
            }
            
            if (need_key_release && tud_hid_ready()) {
                uint8_t empty_keys[6] = {0};
                if (tud_hid_keyboard_report(0, 0, empty_keys)) {
                    printf("Key released\n");
                    need_key_release = false;
                }
            }
            
            // Reset flag when button is released
            if (!current_button && button_pressed) {
                printf("Button released - ready for next press\n");
                button_pressed = false;
                need_key_release = false;
            }
        } else {
            blink_interval_ms = 1000;
        }
        
        sleep_ms(10); // 10ms polling interval
    }
    
    return 0;
}
