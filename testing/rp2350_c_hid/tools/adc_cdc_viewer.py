#!/usr/bin/env python3
"""
ADC CDC Viewer - Reads ADC data from CDC serial port and displays as bar graphs
"""

import tkinter as tk
from tkinter import messagebox
import serial
import serial.tools.list_ports
import threading
import queue
import time

# Constants
BG_COLOR = '#1a1a1a'
BORDER_COLOR = '#333333'
BAR_COLOR_LOW = '#00ffff'    # Cyan
BAR_COLOR_MID = '#ff8800'    # Orange
BAR_COLOR_HIGH = '#ff0000'   # Red
MAX_MV = 3300  # 3.3V reference

NUM_CHANNELS = 80
COLS = 16
ROWS = 5

class CDCReader(threading.Thread):
    def __init__(self, q, stop_event, port):
        super().__init__(daemon=True)
        self.q = q
        self.stop_event = stop_event
        self.port = port
        self.ser = None
        self.opened = False

    def open(self):
        try:
            self.ser = serial.Serial(self.port, 115200, timeout=0.1)
            self.opened = True
            self.q.put(("info", f"Opened CDC port {self.port}"))
            print(f"Successfully opened {self.port}")
        except Exception as e:
            self.q.put(("error", f"Failed to open serial port: {e}"))
            print(f"Failed to open serial: {e}")
            self.opened = False

    def run(self):
        self.open()
        if not self.opened:
            return

        line_count = 0
        in_adc_block = False
        channel_data = {}
        print("CDC reader thread started, waiting for data...")
        
        while not self.stop_event.is_set():
            try:
                if self.ser.in_waiting:
                    line = self.ser.readline().decode('utf-8', errors='ignore').strip()
                    
                    if not line:
                        continue
                    
                    line_count += 1
                    
                    # Check for ADC block markers
                    if line == "===ADC_START===":
                        in_adc_block = True
                        channel_data = {}
                        continue
                    elif line == "===ADC_END===":
                        in_adc_block = False
                        # Convert to ordered list
                        if len(channel_data) == NUM_CHANNELS:
                            values = [channel_data.get(i, 0) for i in range(NUM_CHANNELS)]
                            self.q.put(("payload", values))
                            if line_count % 100 == 1:
                                self.q.put(("info", f"✅ Parsed {NUM_CHANNELS} channels"))
                        continue
                    
                    # Parse channel data: "CH 0:1234"
                    if in_adc_block and line.startswith("CH "):
                        try:
                            parts = line[3:].split(':')
                            if len(parts) == 2:
                                ch_num = int(parts[0])
                                mv = int(parts[1])
                                if 0 <= ch_num < NUM_CHANNELS:
                                    channel_data[ch_num] = mv
                        except ValueError:
                            pass
                else:
                    time.sleep(0.01)
                    
            except Exception as e:
                error_msg = str(e)
                print(f"Serial read exception: {error_msg}")
                self.q.put(("error", f"Serial read error: {error_msg}"))
                time.sleep(0.1)
                continue
        
        try:
            if self.ser:
                self.ser.close()
        except Exception:
            pass

class CDCViewer(tk.Tk):
    def __init__(self):
        super().__init__()
        self.title("🎛️ ADC CDC Viewer - Press any key to scan")
        self.geometry("1400x800")
        self.configure(bg=BG_COLOR)

        self.cols = COLS
        self.rows = ROWS

        self.q = queue.Queue()
        self.stop_event = threading.Event()
        self.reader = None
        self.bar_items = []

        # Find CDC port
        self.cdc_port = self.find_cdc_port()
        if not self.cdc_port:
            messagebox.showerror("No CDC Port", "Could not find Pico CDC serial port!")
            self.destroy()
            return

        # Top bar with connection controls
        top_frame = tk.Frame(self, bg=BG_COLOR, height=60)
        top_frame.pack(side=tk.TOP, fill=tk.X)
        top_frame.pack_propagate(False)

        self.connect_btn = tk.Button(
            top_frame, text="🔴 Disconnect CDC", bg='#aa2222', fg='white',
            font=('Arial', 12, 'bold'), command=self.toggle, relief=tk.FLAT
        )
        self.connect_btn.pack(side=tk.LEFT, padx=10, pady=10)

        self.status_var = tk.StringVar(value=f"🟡 Connecting to {self.cdc_port}...")
        status_lbl = tk.Label(
            top_frame, textvariable=self.status_var, bg=BG_COLOR, fg='white',
            font=('Arial', 11), anchor='w'
        )
        status_lbl.pack(side=tk.LEFT, padx=10, fill=tk.X, expand=True)

        channel_info = tk.Label(
            top_frame, text=f"80 Channels (5 MUX × 16)", bg=BG_COLOR, fg='#888888',
            font=('Arial', 10)
        )
        channel_info.pack(side=tk.RIGHT, padx=10)

        # Canvas for grid
        self.canvas = tk.Canvas(self, bg=BG_COLOR, highlightthickness=0)
        self.canvas.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)

        self.bind('<Configure>', lambda e: self.draw_grid())
        self.bind('<Key>', self.on_key_press)

        self.draw_grid()
        self.process_queue()

        # Auto-connect
        self.reader = CDCReader(self.q, self.stop_event, self.cdc_port)
        self.reader.start()

    def find_cdc_port(self):
        """Find the Pico's CDC serial port - hardcoded to COM46"""
        return 'COM46'

    def on_key_press(self, event):
        if self.reader and self.reader.is_alive():
            self.status_var.set(f"🎹 Key '{event.char}' pressed - Scanning...")
            self.after(1000, lambda: self.status_var.set(f"🟢 Connected to {self.cdc_port}"))

    def toggle(self):
        if self.reader and self.reader.is_alive():
            self.stop_event.set()
            self.connect_btn.config(text="🔌 Connect CDC", bg='#2a2a2a')
            self.status_var.set("⚪ Disconnecting...")
        else:
            self.stop_event.clear()
            self.reader = CDCReader(self.q, self.stop_event, self.cdc_port)
            self.reader.start()
            self.connect_btn.config(text="🔴 Disconnect CDC", bg='#aa2222')
            self.status_var.set(f"🟡 Connecting to {self.cdc_port}...")

    def draw_grid(self):
        self.canvas.delete("all")
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
        
        # Draw MUX labels
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
                
                # Background cell
                self.canvas.create_rectangle(
                    x0, y0, x1, y1, 
                    outline=BORDER_COLOR, width=1, fill='#0d0d0d'
                )
                
                # Bar
                bar = self.canvas.create_rectangle(
                    x0+2, y1-2, x1-2, y1-2, 
                    outline='', fill=BAR_COLOR_LOW, width=0
                )
                
                # Channel label
                ch_num = r * self.cols + c
                label = self.canvas.create_text(
                    (x0+x1)/2, y0+8, 
                    text=f"{ch_num}", 
                    fill='#444444', font=('Arial', 7)
                )
                
                # Value text
                txt = self.canvas.create_text(
                    (x0+x1)/2, (y0+y1)/2 + 5, 
                    text="-", 
                    fill='#333333', font=('Arial', 8, 'bold')
                )
                
                row_items.append((bar, x0, y0, x1, y1, txt, label))
            self.bar_items.append(row_items)

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
                    if "Opened CDC" in info_msg:
                        self.status_var.set(f"🟢 Connected to {self.cdc_port}")
                    else:
                        self.status_var.set(info_msg)
                elif kind == 'error':
                    messagebox.showerror('CDC Error', payload)
                    self.status_var.set('🔴 Error')
        except queue.Empty:
            pass
        self.after(50, self.process_queue)

    def update_grid(self, values):
        """Update grid with bar graphs"""
        if not self.bar_items or len(self.bar_items) == 0:
            return
            
        for i, mv in enumerate(values):
            r = i // self.cols
            c = i % self.cols
            if r >= len(self.bar_items) or c >= len(self.bar_items[r]):
                continue
                
            bar, x0, y0, x1, y1, txt, label = self.bar_items[r][c]
            
            # Calculate bar height
            cell_height = y1 - y0
            frac = min(max(mv / float(MAX_MV), 0.0), 1.0)
            bar_height = (cell_height - 20) * frac
            new_y = y1 - 2 - bar_height
            
            # Update bar
            self.canvas.coords(bar, x0+2, new_y, x1-2, y1-2)
            self.canvas.itemconfig(bar, fill=self.get_bar_color(mv))
            
            # Update text
            if mv > 0:
                self.canvas.itemconfig(txt, text=f"{mv}", fill='white')
            else:
                self.canvas.itemconfig(txt, text="-", fill='#333333')

    def redraw_if_needed(self):
        # Redraw if window was resized
        pass

if __name__ == '__main__':
    app = CDCViewer()
    app.mainloop()
