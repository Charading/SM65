MCU = RP2040
BOOTLOADER = rp2040

# Enable Raspberry Pi Pico SDK for RP2040-specific APIs used by wireless.c
PICO_SDK := yes

SRC += wireless.c
