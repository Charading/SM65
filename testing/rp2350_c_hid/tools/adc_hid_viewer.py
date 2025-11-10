"""
HID-based ADC Viewer

Reads binary ADC payloads from the vendor HID interface (VID=0xCAFE, PID=0x4001, report ID=2).
Firmware sends 160 bytes = 80 uint16 little-endian millivolt values split into 64-byte HID reports.

Beautiful dark UI with gradient bars that update when you press any key.

Requires: hid (hidapi) and tkinter
Run: python tools/adc_hid_viewer.py
"""

import hid
import threading
import queue
import struct
import time
import tkinter as tk
from tkinter import ttk, messagebox

VID = 0xCAFE
PID = 0x4001
REPORT_ID = 2
NUM_CHANNELS = 80
ROWS = 5
COLS = 16
MAX_MV = 3300

# Color scheme for beautiful UI
BG_COLOR = '#1a1a1a'
GRID_BG = '#0a0a0a'
BAR_COLOR_LOW = '#00ff88'
BAR_COLOR_MID = '#ffaa00'
BAR_COLOR_HIGH = '#ff3366'
TEXT_COLOR = '#ffffff'
BORDER_COLOR = '#333333'

class HIDReader(threading.Thread):
    def __init__(self, q, stop_event):
        super().__init__(daemon=True)
        self.q = q
        self.stop_event = stop_event
        self.dev = None
        self.opened = False

    def open(self):
        try:
            # Find the vendor HID interface (MI_03, not the keyboard MI_02)
            devices = hid.enumerate(VID, PID)
            vendor_path = None
            
            for dev in devices:
                path_str = dev['path'].decode('utf-8') if isinstance(dev['path'], bytes) else str(dev['path'])
                print(f"Found: {path_str}")
                # Look for MI_03 (interface 3 = vendor HID)
                # Avoid KBD suffix which indicates keyboard interface
                if 'MI_03' in path_str and not path_str.endswith('\\KBD'):
                    vendor_path = dev['path']
                    print(f"  -> This is the vendor HID interface!")
                    break
            
            if not vendor_path:
                raise Exception("Vendor HID interface (MI_03) not found")
            
            self.dev = hid.device()
            self.dev.open_path(vendor_path)
            self.dev.set_nonblocking(False)  # Use blocking mode for Windows
            self.opened = True
            self.q.put(("info", f"Opened vendor HID {VID:04x}:{PID:04x}"))
            print(f"Successfully opened vendor HID interface")
        except Exception as e:
            self.q.put(("error", f"Failed to open HID device: {e}"))
            print(f"Failed to open HID: {e}")
            self.opened = False

    def run(self):
        self.open()
        if not self.opened:
            return

        buf_acc = bytearray()
        read_count = 0
        print("HID reader thread started, waiting for data...")
        while not self.stop_event.is_set():
            try:
                # Read with timeout (in milliseconds for blocking mode)
                data = self.dev.read(65, timeout_ms=100)  # report id + 64
                if not data:
                    continue
            except Exception as e:
                error_msg = str(e)
                print(f"HID read exception: {error_msg}")
                self.q.put(("error", f"HID read error: {error_msg}"))
                # Don't break immediately, try to recover
                time.sleep(0.1)
                continue
            
            read_count += 1
            # data may be list of ints
            if isinstance(data, list):
                data = bytes(data)
            
            # Debug: show we're receiving data
            if read_count % 10 == 1:
                self.q.put(("info", f"📦 Received {len(data)} bytes (total reads: {read_count})"))
            
            # first byte is report id (if present)
            if len(data) == 0:
                continue
            report_id = data[0]
            if report_id != REPORT_ID:
                # ignore other reports, but log them
                if read_count % 10 == 1:
                    self.q.put(("info", f"⚠️ Ignoring report ID {report_id}, expecting {REPORT_ID}"))
                continue
            
            chunk = data[1:]
            buf_acc.extend(chunk)
            
            # If we've accumulated at least 160 bytes, emit payload and reset
            if len(buf_acc) >= NUM_CHANNELS * 2:
                payload = bytes(buf_acc[:NUM_CHANNELS*2])
                buf_acc = buf_acc[NUM_CHANNELS*2:]
                # parse payload into 80 uint16 little-endian
                vals = list(struct.unpack('<' + 'H'*NUM_CHANNELS, payload))
                self.q.put(("payload", vals))
                self.q.put(("info", f"✅ Parsed {NUM_CHANNELS} channels"))
        try:
            if self.dev:
                self.dev.close()
        except Exception:
            pass

class HIDViewer(tk.Tk):
    def __init__(self):
        super().__init__()
        self.title("🎛️ ADC HID Viewer - Press any key to scan")
        self.geometry("1400x800")
        self.configure(bg=BG_COLOR)

        # Top control bar with dark theme
        ctrl = tk.Frame(self, bg=BG_COLOR, pady=10)
        ctrl.pack(side=tk.TOP, fill=tk.X)
        
        self.connect_btn = tk.Button(
            ctrl, text="🔌 Connect HID", command=self.toggle,
            bg='#2a2a2a', fg=TEXT_COLOR, relief=tk.FLAT, padx=20, pady=8,
            font=('Arial', 10, 'bold'), cursor='hand2'
        )
        self.connect_btn.pack(side=tk.LEFT, padx=10)
        
        self.status_var = tk.StringVar(value="⚪ Disconnected - Press any key to scan")
        status_label = tk.Label(
            ctrl, textvariable=self.status_var,
            bg=BG_COLOR, fg=TEXT_COLOR, font=('Arial', 10)
        )
        status_label.pack(side=tk.LEFT, padx=10)
        
        # Info label
        info = tk.Label(
            ctrl, text="80 Channels (5 MUX × 16)",
            bg=BG_COLOR, fg='#888888', font=('Arial', 9, 'italic')
        )
        info.pack(side=tk.RIGHT, padx=10)

        # Canvas with dark background
        self.canvas = tk.Canvas(self, bg=GRID_BG, highlightthickness=0)
        self.canvas.pack(side=tk.TOP, fill=tk.BOTH, expand=True, padx=10, pady=10)

        self.rows = ROWS
        self.cols = COLS
        self.bar_items = []
        self.last_values = [0] * NUM_CHANNELS
        self.draw_grid()

        self.q = queue.Queue()
        self.stop_event = threading.Event()
        self.reader = None
        
        # Bind key presses to trigger scans
        self.bind('<Key>', self.on_key_press)
        self.bind('<Configure>', self.on_resize)
        self.focus_set()

        self.after(50, self.process_queue)
    
    def redraw_if_needed(self):
        """Redraw grid if canvas size has changed"""
        self.canvas.update_idletasks()
        if self.canvas.winfo_width() > 100 and not self.bar_items:
            self.draw_grid()
    
    def on_resize(self, event):
        """Redraw grid when window is resized"""
        if event.widget == self and self.bar_items:
            self.after(100, self.draw_grid)

    def on_key_press(self, event):
        """When any key is pressed, request a scan (visual feedback)"""
        if self.reader and self.reader.is_alive():
            # Flash the status to show key was received
            self.status_var.set(f"🎹 Key '{event.char}' pressed - Scan requested")
            # Note: Actual scan triggering happens when firmware sends data
            # Could implement OUT report here if firmware supports it
            self.after(1000, lambda: self.status_var.set("🟢 Connected - Press any key to scan"))
    
    def toggle(self):
        if self.reader and self.reader.is_alive():
            self.stop_event.set()
            self.connect_btn.config(text="🔌 Connect HID", bg='#2a2a2a')
            self.status_var.set("⚪ Disconnecting...")
        else:
            self.stop_event.clear()
            self.reader = HIDReader(self.q, self.stop_event)
            self.reader.start()
            self.connect_btn.config(text="🔴 Disconnect HID", bg='#aa2222')
            self.status_var.set("🟡 Connecting...")

    def draw_grid(self):
        self.canvas.delete("all")
        # Force canvas to update to get actual size
        self.canvas.update_idletasks()
        w = self.canvas.winfo_width()
        h = self.canvas.winfo_height()
        if w < 100:
            w = 1360
        if h < 100:
            h = 680
            
        margin_left = 50
        margin_top = 20
        margin_right = 20
        margin_bottom = 20
        
        grid_w = w - margin_left - margin_right
        grid_h = h - margin_top - margin_bottom
        cell_w = grid_w / self.cols
        cell_h = grid_h / self.rows
        
        # Draw MUX labels on the left
        for r in range(self.rows):
            y_mid = margin_top + (r + 0.5) * cell_h
            self.canvas.create_text(
                10, y_mid, text=f"MUX{r+1}", 
                fill='#888888', font=('Arial', 10, 'bold'), anchor='w'
            )
        
        self.bar_items = []
        for r in range(self.rows):
            row_items = []
            for c in range(self.cols):
                x0 = margin_left + c*cell_w + 3
                y0 = margin_top + r*cell_h + 3
                x1 = margin_left + (c+1)*cell_w - 3
                y1 = margin_top + (r+1)*cell_h - 3
                
                # Background cell with border
                self.canvas.create_rectangle(
                    x0, y0, x1, y1, 
                    outline=BORDER_COLOR, width=1, fill='#0d0d0d'
                )
                
                # Bar (starts at bottom, grows up)
                bar = self.canvas.create_rectangle(
                    x0+2, y1-2, x1-2, y1-2, 
                    outline='', fill=BAR_COLOR_LOW, width=0
                )
                
                # Channel label at top
                ch_num = r * self.cols + c
                label = self.canvas.create_text(
                    (x0+x1)/2, y0+8, 
                    text=f"{ch_num}", 
                    fill='#444444', font=('Arial', 7)
                )
                
                # Value text in center
                txt = self.canvas.create_text(
                    (x0+x1)/2, (y0+y1)/2 + 5, 
                    text="-", 
                    fill='#333333', font=('Arial', 8, 'bold')
                )
                
                row_items.append((bar, x0, y0, x1, y1, txt, label))
            self.bar_items.append(row_items)
        
        # Redraw after window is shown
        self.after(100, self.redraw_if_needed)

    def get_bar_color(self, mv):
        """Get gradient color based on millivolt value"""
        if mv == 0:
            return BG_COLOR
        frac = mv / float(MAX_MV)
        if frac < 0.33:
            return BAR_COLOR_LOW
        elif frac < 0.66:
            return BAR_COLOR_MID
        else:
            return BAR_COLOR_HIGH
    
    def process_queue(self):
        try:
            while True:
                kind, payload = self.q.get_nowait()
                if kind == 'payload':
                    self.update_grid(payload)
                elif kind == 'info':
                    info_msg = payload
                    if "Opened HID" in info_msg:
                        self.status_var.set("🟢 Connected - Press any key to scan")
                    else:
                        self.status_var.set(info_msg)
                elif kind == 'error':
                    messagebox.showerror('HID Error', payload)
                    self.status_var.set('🔴 Error')
        except queue.Empty:
            pass
        self.after(50, self.process_queue)

    def update_grid(self, values):
        """Update grid with smooth animation and gradient colors"""
        if not self.bar_items or len(self.bar_items) == 0:
            return
            
        for i, mv in enumerate(values):
            r = i // self.cols
            c = i % self.cols
            if r >= len(self.bar_items) or c >= len(self.bar_items[r]):
                continue
                
            bar, x0, y0, x1, y1, txt, label = self.bar_items[r][c]
            
            # Calculate bar height with padding
            cell_height = y1 - y0
            frac = min(max(mv / float(MAX_MV), 0.0), 1.0)
            bar_height = (cell_height - 20) * frac  # Leave space for text
            new_y = y1 - 2 - bar_height
            
            # Update bar geometry
            self.canvas.coords(bar, x0+2, new_y, x1-2, y1-2)
            
            # Update bar color based on value
            color = self.get_bar_color(mv)
            self.canvas.itemconfig(bar, fill=color)
            
            # Update text
            if mv > 0:
                if mv > 999:
                    display = f"{mv/1000:.1f}V"
                else:
                    display = f"{mv}"
                self.canvas.itemconfig(txt, text=display, fill=TEXT_COLOR)
            else:
                self.canvas.itemconfig(txt, text="-", fill='#333333')
            
            # Highlight label if active
            if mv > 0:
                self.canvas.itemconfig(label, fill='#888888')
            else:
                self.canvas.itemconfig(label, fill='#333333')
        
        # Update status
        active_count = sum(1 for v in values if v > 0)
        self.status_var.set(f"🟢 Active: {active_count}/{NUM_CHANNELS} channels - Press any key to scan")
        self.last_values = values

if __name__ == '__main__':
    app = HIDViewer()
    app.mainloop()
