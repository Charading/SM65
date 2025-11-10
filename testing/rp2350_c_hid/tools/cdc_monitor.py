"""
Simple CDC monitor to see debug messages from the device
"""
import serial
import serial.tools.list_ports

# Find the CDC port
print("Looking for CDC port...")
for port in serial.tools.list_ports.comports():
    if "CAFE" in port.hwid.upper() or "4001" in port.hwid:
        print(f"Found: {port.device} - {port.description}")
        print(f"  HWID: {port.hwid}")
        
        try:
            ser = serial.Serial(port.device, 115200, timeout=1)
            print(f"\nConnected to {port.device} at 115200 baud")
            print("Monitoring output (Ctrl+C to stop)...\n")
            print("-" * 80)
            
            while True:
                if ser.in_waiting:
                    data = ser.read(ser.in_waiting)
                    try:
                        print(data.decode('utf-8'), end='')
                    except:
                        print(data)
        except KeyboardInterrupt:
            print("\nStopped")
            ser.close()
            break
        except Exception as e:
            print(f"Error: {e}")
        break
else:
    print("CDC port not found!")
    print("\nAll available ports:")
    for port in serial.tools.list_ports.comports():
        print(f"  {port.device}: {port.description}")
