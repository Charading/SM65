"""
List all HID devices to verify our device is connected
"""
import hid

print("Enumerating all HID devices...")
print("-" * 80)

devices = hid.enumerate()
found_our_device = False

for dev in devices:
    vid = dev['vendor_id']
    pid = dev['product_id']
    manufacturer = dev['manufacturer_string']
    product = dev['product_string']
    path = dev['path']
    
    print(f"VID: 0x{vid:04X}  PID: 0x{pid:04X}")
    print(f"  Manufacturer: {manufacturer}")
    print(f"  Product:      {product}")
    print(f"  Path:         {path}")
    print()
    
    if vid == 0xCAFE and pid == 0x4001:
        found_our_device = True
        print("  ★★★ THIS IS OUR DEVICE! ★★★")
        print()

print("-" * 80)
if found_our_device:
    print("✓ Found device VID=0xCAFE PID=0x4001")
else:
    print("✗ Device VID=0xCAFE PID=0x4001 NOT FOUND")
    print("\nTroubleshooting:")
    print("1. Make sure the RP2350 is plugged in via USB")
    print("2. Try unplugging and replugging the device")
    print("3. Check if Windows recognizes it in Device Manager")
    print("4. Make sure the firmware is flashed correctly")
