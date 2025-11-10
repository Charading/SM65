"""
Simple Tkinter GUI to display ADC channels streamed from the RP2350 over USB CDC (serial).

Protocol expected from firmware: CSV lines like
  <timestamp_ms>,<ch0_mV>,<ch1_mV>,...,<ch79_mV>\n
Values of 0 indicate below threshold (ignored / floating channels).

Requires: pyserial
Run: python tools/adc_viewer.py
"""

import threading
import queue
import time
import serial
import serial.tools.list_ports
import tkinter as tk
from tkinter import ttk
from tkinter import messagebox

PORT_POLL_INTERVAL = 2.0
SERIAL_BAUD = 115200
NUM_CHANNELS = 80
MUXES = 5
CHANNELS_PER_MUX = 16
MAX_MV = 3300

class SerialReader(threading.Thread):
    def __init__(self, port, baud, q, stop_event):
        super().__init__(daemon=True)
        self.port = port
        self.baud = baud
        self.q = q
        self.stop_event = stop_event
        self.ser = None

    def run(self):
        try:
            self.ser = serial.Serial(self.port, self.baud, timeout=1)
        except Exception as e:
            self.q.put(("error", f"Failed to open {self.port}: {e}"))
            return

        self.q.put(("info", f"Opened {self.port} @ {self.baud}"))

        buf = b""
        while not self.stop_event.is_set():
            try:
                data = self.ser.read(256)
            except Exception as e:
                self.q.put(("error", f"Serial read error: {e}"))
                break
            if not data:
                continue
            buf += data
            while b"\n" in buf:
                line, buf = buf.split(b"\n", 1)
                line = line.decode(errors='ignore').strip()
                if not line:
                    continue
                self.q.put(("line", line))
        try:
            if self.ser and self.ser.is_open:
                self.ser.close()
        except Exception:
            pass

class ADCViewer(tk.Tk):
    def __init__(self):
        super().__init__()
        self.title("ADC Viewer")
        self.geometry("1200x600")

        # Serial controls
        ctrl = ttk.Frame(self)
        ctrl.pack(side=tk.TOP, fill=tk.X)

        ttk.Label(ctrl, text="Serial Port:").pack(side=tk.LEFT, padx=4)
        self.port_var = tk.StringVar()
        self.port_box = ttk.Combobox(ctrl, textvariable=self.port_var, width=30)
        self.port_box.pack(side=tk.LEFT, padx=4)

        self.refresh_btn = ttk.Button(ctrl, text="Refresh", command=self.refresh_ports)
        self.refresh_btn.pack(side=tk.LEFT, padx=4)

        self.connect_btn = ttk.Button(ctrl, text="Connect", command=self.toggle_connect)
        self.connect_btn.pack(side=tk.LEFT, padx=4)

        self.status_var = tk.StringVar(value="Not connected")
        ttk.Label(ctrl, textvariable=self.status_var).pack(side=tk.LEFT, padx=8)

        # Canvas for bars
        self.canvas = tk.Canvas(self)
        self.canvas.pack(side=tk.TOP, fill=tk.BOTH, expand=True)

        # Create progress bars grid (5 rows x 16 columns)
        self.bars = []  # list of (rect, text)
        self.rows = MUXES
        self.cols = CHANNELS_PER_MUX

        # queue for serial lines
        self.q = queue.Queue()
        self.stop_event = threading.Event()
        self.reader = None

        self.refresh_ports()
        self.draw_grid()
        # bind keypress to request scans
        self.bind('<Key>', self.on_keypress)
        self.after(100, self.process_queue)

    def refresh_ports(self):
        ports = [p.device for p in serial.tools.list_ports.comports()]
        self.port_box['values'] = ports
        if ports and not self.port_var.get():
            self.port_var.set(ports[0])

    def toggle_connect(self):
        if self.reader and self.reader.is_alive():
            self.stop_event.set()
            self.connect_btn.config(text="Connect")
            self.status_var.set("Disconnecting...")
        else:
            port = self.port_var.get()
            if not port:
                messagebox.showwarning("No port", "Select a serial port first")
                return
            self.stop_event.clear()
            self.reader = SerialReader(port, SERIAL_BAUD, self.q, self.stop_event)
            self.reader.start()
            self.connect_btn.config(text="Disconnect")
            self.status_var.set(f"Connecting to {port}...")

    def on_keypress(self, event):
        # When any key is pressed in the GUI, request a fresh scan from device
        if self.reader and self.reader.ser and self.reader.ser.is_open:
            try:
                # send single 's' to request scan
                self.reader.ser.write(b's')
            except Exception as e:
                self.q.put(("error", f"Failed to send scan request: {e}"))

    def draw_grid(self):
        self.canvas.delete("all")
        w = self.canvas.winfo_width() or 1000
        h = self.canvas.winfo_height() or 400
        margin = 10
        grid_w = w - 2*margin
        grid_h = h - 2*margin
        cell_w = grid_w / self.cols
        cell_h = grid_h / self.rows
        self.bar_items = []
        for r in range(self.rows):
            row_items = []
            for c in range(self.cols):
                x0 = margin + c*cell_w + 4
                y0 = margin + r*cell_h + 10
                x1 = margin + (c+1)*cell_w - 4
                y1 = margin + (r+1)*cell_h - 10
                # background rect
                self.canvas.create_rectangle(x0, y0, x1, y1, outline='#444', fill='#222')
                # foreground rect (value)
                bar = self.canvas.create_rectangle(x0, y1, x1, y1, outline='', fill='#0a0')
                txt = self.canvas.create_text((x0+x1)/2, y0+8, text=f"R{r}C{c}", fill='#fff', font=(None,8))
                row_items.append((bar, x0, y0, x1, y1, txt))
            self.bar_items.append(row_items)
        self.update()
    

    def process_queue(self):
        try:
            while True:
                kind, payload = self.q.get_nowait()
                if kind == 'line':
                    self.handle_line(payload)
                elif kind == 'info':
                    self.status_var.set(payload)
                elif kind == 'error':
                    messagebox.showerror("Serial Error", payload)
                    self.status_var.set("Error")
        except queue.Empty:
            pass
        self.after(100, self.process_queue)

    def handle_line(self, line):
        # expect CSV: ts, mV0, mV1, ...
        parts = line.split(',')
        if len(parts) < 1 + NUM_CHANNELS:
            return
        try:
            ts = int(parts[0])
            values = [int(parts[i+1]) for i in range(NUM_CHANNELS)]
        except Exception:
            return
        # update bars
        for i, mv in enumerate(values):
            r = i // self.cols
            c = i % self.cols
            bar, x0, y0, x1, y1, txt = self.bar_items[r][c]
            # compute height based on mv/MAX_MV
            frac = min(max(mv / float(MAX_MV), 0.0), 1.0)
            new_y = y1 - (y1 - y0) * frac
            # update bar coords
            self.canvas.coords(bar, x0, new_y, x1, y1)
            # update text to show mV when active
            if mv:
                self.canvas.itemconfig(txt, text=f"{mv}mV")
            else:
                self.canvas.itemconfig(txt, text="-")
        self.status_var.set(f"Last: {ts} ms")

if __name__ == '__main__':
    try:
        import sys
        app = ADCViewer()
        app.mainloop()
    except KeyboardInterrupt:
        pass
