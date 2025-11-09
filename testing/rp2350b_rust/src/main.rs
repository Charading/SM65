//! SPDX-License-Identifier: MIT OR Apache-2.0
//!
//! Copyright (c) 2021–2024 The rp-rs Developers
//! Copyright (c) 2021 rp-rs organization
//! Copyright (c) 2025 Raspberry Pi Ltd.
//!
//! # GPIO 'Blinky' Example
//!
//! This application demonstrates how to control a GPIO pin on the rp2040 and rp235x.
//!
//! It may need to be adapted to your particular board layout and/or pin assignment.

#![no_std]
#![no_main]

use defmt::*;
use defmt_rtt as _;
use embedded_hal::delay::DelayNs;
// removed unused imports for this example
#[cfg(target_arch = "riscv32")]
use panic_halt as _;
#[cfg(target_arch = "arm")]
use panic_probe as _;

// Alias for our HAL crate
use hal::entry;

#[cfg(rp2350)]
use rp235x_hal as hal;

#[cfg(rp2040)]
use rp2040_hal as hal;

// use bsp::entry;
// use bsp::hal;
// use rp_pico as bsp;

/// The linker will place this boot block at the start of our program image. We
/// need this to help the ROM bootloader get our code up and running.
/// Note: This boot block is not necessary when using a rp-hal based BSP
/// as the BSPs already perform this step.
#[unsafe(link_section = ".boot2")]
#[used]
#[cfg(rp2040)]
pub static BOOT2: [u8; 256] = rp2040_boot2::BOOT_LOADER_W25Q080;

/// Tell the Boot ROM about our application
#[unsafe(link_section = ".start_block")]
#[used]
#[cfg(rp2350)]
pub static IMAGE_DEF: hal::block::ImageDef = hal::block::ImageDef::secure_exe();

/// External high-speed crystal on the Raspberry Pi Pico 2 board is 12 MHz.
/// Adjust if your board has a different frequency
const XTAL_FREQ_HZ: u32 = 12_000_000u32;

/// Entry point to our bare-metal application.
///
/// This example exposes a simple USB HID keyboard device. It sends a single 'a' keypress
/// periodically so the host can detect the device and you can verify HID functionality.
#[entry]
fn main() -> ! {
    info!("HID keyboard example start");

    // Grab our singleton objects
    let mut pac = hal::pac::Peripherals::take().unwrap();

    // Set up the watchdog driver - needed by the clock setup code
    let mut watchdog = hal::Watchdog::new(pac.WATCHDOG);

    // Configure the clocks
    let clocks = hal::clocks::init_clocks_and_plls(
        XTAL_FREQ_HZ,
        pac.XOSC,
        pac.CLOCKS,
        pac.PLL_SYS,
        pac.PLL_USB,
        &mut pac.RESETS,
        &mut watchdog,
    )
    .unwrap();

    // Small delay/timer depending on platform
    #[cfg(rp2040)]
    let mut timer = hal::Timer::new(pac.TIMER, &mut pac.RESETS, &clocks);

    #[cfg(rp2350)]
    let mut timer = hal::Timer::new_timer0(pac.TIMER0, &mut pac.RESETS, &clocks);

    // Create the USB bus allocator from the HAL USB implementation. The rp2040 and rp235x
    // HALs expose a `usb` module with a `UsbBus` type; we conditionally use the correct one.
    // Import UsbBusAllocator from the class prelude for this usb-device version.
    use usb_device::class_prelude::UsbBusAllocator;
    use usb_device::prelude::{UsbDeviceBuilder, UsbVidPid};
    use usbd_hid::descriptor::{KeyboardReport, SerializedDescriptor};
    use usbd_hid::hid_class::HIDClass;

    // Construct USB bus allocator for each platform. The exact constructor for the HAL's
    // UsbBus may differ; the HALs usually expose `hal::usb::UsbBus` with a `new` function.
    // If compilation fails here, we'll inspect the HAL API and adapt the arguments.
    #[cfg(rp2040)]
    let usb_bus: UsbBusAllocator<_> = {
        let usb = hal::usb::UsbBus::new(pac.USB, &mut pac.RESETS, &clocks);
        UsbBusAllocator::new(usb)
    };

    #[cfg(rp2350)]
    let usb_bus: UsbBusAllocator<_> = {
        // rp235x HAL's UsbBus::new requires DPRAM and a UsbClock in addition to the
        // USB peripheral and resets. Obtain the DPRAM from the PAC and extract the
        // UsbClock from the configured clocks manager. The boolean argument selects
        // host/device mode behaviour (use false to force device mode here).
        let usb_dpram = pac.USB_DPRAM;
        // Extract the USB clock from the clocks manager
        let usb_clock = clocks.usb_clock;
        let usb = hal::usb::UsbBus::new(pac.USB, usb_dpram, usb_clock, false, &mut pac.RESETS);
        UsbBusAllocator::new(usb)
    };

    let mut hid = HIDClass::new(&usb_bus, KeyboardReport::desc(), 10);

    // Use minimal builder form (some usb-device versions expose fewer builder methods).
    let mut usb_dev = UsbDeviceBuilder::new(&usb_bus, UsbVidPid(0x1209, 0x0001)).build();

    // Main loop: poll USB and periodically send an 'a' keypress report.
    let mut toggle = false;
    loop {
        if usb_dev.poll(&mut [&mut hid]) {
            // usb events handled inside poll
        }

        // Send a report every ~500ms: press 'a' then release.
        if toggle {
            let report = KeyboardReport {
                modifier: 0,
                reserved: 0,
                leds: 0,
                keycodes: [4, 0, 0, 0, 0, 0], // HID usage ID 4 == 'a'
            };
            let _ = hid.push_input(&report);
        } else {
            // release all keys
            let report = KeyboardReport {
                modifier: 0,
                reserved: 0,
                leds: 0,
                keycodes: [0; 6],
            };
            let _ = hid.push_input(&report);
        }

        toggle = !toggle;

        // platform-agnostic small delay
        timer.delay_ms(500);
    }
}

/// Program metadata for `picotool info`
#[unsafe(link_section = ".bi_entries")]
#[used]
pub static PICOTOOL_ENTRIES: [hal::binary_info::EntryAddr; 5] = [
    hal::binary_info::rp_cargo_bin_name!(),
    hal::binary_info::rp_cargo_version!(),
    hal::binary_info::rp_program_description!(c"Blinky Example"),
    hal::binary_info::rp_cargo_homepage_url!(),
    hal::binary_info::rp_program_build_attribute!(),
];

// End of file
