#!/usr/bin/env python3
"""
Simple serial GUI for viewing MCP3208+HC4067 scans.
- Reads blocks from a USB serial device (USB CDC) where the Pico prints one block
  terminated by a line of dashes ("-----------").
- Parses lines like: "MUX 1 | CH1: 4096 | CH3: 3972 |"
- Displays a 5x8 grid (5 mux rows, 8 columns) as bar graphs scaled 0..4095.

Assumptions:
- There are 5 muxes (rows). Each mux's relevant channels are CH1..CH8 mapped to columns 0..7.
  If your wiring uses different channel indices, update the parsing logic.
- The Pico prints each full scan as a block ending with a line that starts with '-' characters.
- Serial baud defaults to 115200.

Dependencies:
- pyserial (pip install pyserial)
- tkinter (usually included with Python)

Usage:
  python serial_gui.py

Select the serial port from the dropdown and press Connect. When connected,
incoming scan blocks update the grid. Click a cell to highlight it (placeholder
for actuation)."""

import tkinter as tk
from tkinter import ttk, messagebox
import threading
import queue
import re
import time
import serial
import serial.tools.list_ports

# GUI / parsing configuration
ROWS = 5
COLS = 8
MAX_ADC = 4095.0
BLOCK_SEPARATOR = "-" * 3  # we detect lines starting with dashes

# Regular expression to parse MUX lines and channel:value pairs
RE_MUX = re.compile(r"^\s*MUX\s*(\d+)", re.IGNORECASE)
RE_CHVAL = re.compile(r"CH(\d+):\s*(\d+)")

# Thread-safe queue to pass parsed blocks to GUI
block_queue = queue.Queue()


def list_serial_ports():
    return [p.device for p in serial.tools.list_ports.comports()]


class SerialReader(threading.Thread):
    def __init__(self, port, baud, queue):
        super().__init__(daemon=True)
        self.port = port
        self.baud = baud
        self.queue = queue
        self._stop = threading.Event()
        self.ser = None

    def stop(self):
        self._stop.set()
        if self.ser and self.ser.is_open:
            try:
                self.ser.close()
            except Exception:
                pass

    def run(self):
        try:
            self.ser = serial.Serial(self.port, self.baud, timeout=1)
        except Exception as e:
            self.queue.put(("__error__", str(e)))
            return

        buf_lines = []
        try:
            while not self._stop.is_set():
                line = self.ser.readline()
                if not line:
                    continue
                try:
                    text = line.decode('utf-8', errors='replace').rstrip('\r\n')
                except Exception:
                    text = str(line)
                # collect lines until separator
                if text.strip().startswith(BLOCK_SEPARATOR[0]):
                    # end of block
                    if buf_lines:
                        block = '\n'.join(buf_lines)
                        self.queue.put(("__block__", block))
                        buf_lines = []
                else:
                    buf_lines.append(text)
        except Exception as e:
            self.queue.put(("__error__", str(e)))
        finally:
            if self.ser and self.ser.is_open:
                try:
                    self.ser.close()
                except Exception:
                    pass


class MuxGridApp:
    def __init__(self, root):
        self.root = root
        root.title("Mux ADC Grid Monitor")

        # Serial controls
        top = ttk.Frame(root)
        top.pack(side=tk.TOP, fill=tk.X, padx=6, pady=6)

        ttk.Label(top, text="Port:").pack(side=tk.LEFT)
        self.port_var = tk.StringVar()
        self.port_combo = ttk.Combobox(top, textvariable=self.port_var, values=list_serial_ports(), width=20)
        self.port_combo.pack(side=tk.LEFT, padx=(4, 8))

        ttk.Label(top, text="Baud:").pack(side=tk.LEFT)
        self.baud_var = tk.IntVar(value=115200)
        self.baud_entry = ttk.Entry(top, textvariable=self.baud_var, width=8)
        self.baud_entry.pack(side=tk.LEFT, padx=(4, 8))

        self.connect_btn = ttk.Button(top, text="Connect", command=self.toggle_connect)
        self.connect_btn.pack(side=tk.LEFT)

        self.status_var = tk.StringVar(value="Disconnected")
        ttk.Label(top, textvariable=self.status_var).pack(side=tk.LEFT, padx=(12,0))

        # Canvas grid
        self.canvas = tk.Canvas(root, width=800, height=400, bg="#222222")
        self.canvas.pack(fill=tk.BOTH, expand=True, padx=6, pady=6)
        self.canvas.bind('<Configure>', self.on_resize)

        # grid cell data
        self.cell_rects = [[None]*COLS for _ in range(ROWS)]
        # store (label_text_id, value_text_id)
        self.cell_texts = [[(None, None) for _ in range(COLS)] for _ in range(ROWS)]
        self.cell_values = [[0]*COLS for _ in range(ROWS)]

        # margin between cells
        self.cell_margin = 8
        # header space inside each cell reserved for CH label
        self.cell_header_h = 20
        # list for MUX row labels
        self.row_labels = [None] * ROWS

        self.create_grid()

        # serial reader
        self.reader = None
        self.root.after(200, self.poll_queue)

    def create_grid(self):
        self.canvas.delete('all')
        w = self.canvas.winfo_width()
        h = self.canvas.winfo_height()
        if w < 10 or h < 10:
            return
        cols = COLS
        rows = ROWS
        cw = max(60, (w - (cols+1)*self.cell_margin) // cols)
        ch = max(48, (h - (rows+1)*self.cell_margin) // rows)
        self.cell_w = cw
        self.cell_h = ch

        for r in range(rows):
            # draw MUX label to the left of the row
            label_x = self.cell_margin // 2
            label_y = self.cell_margin + r*(ch + self.cell_margin) + ch//2
            if self.row_labels[r] is not None:
                try:
                    self.canvas.delete(self.row_labels[r])
                except Exception:
                    pass
            self.row_labels[r] = self.canvas.create_text(label_x, label_y, anchor='w', fill='#ffffff', text=f'MUX {r+1}')

            for c in range(cols):
                x = self.cell_margin + c*(cw + self.cell_margin)
                y = self.cell_margin + r*(ch + self.cell_margin)
                # background rectangle
                rect = self.canvas.create_rectangle(x, y, x+cw, y+ch, fill="#333333", outline="#555555", width=2)
                # inner bar (initially zero height) - bar area is below header
                bar_top = y + self.cell_header_h + 8
                bar = self.canvas.create_rectangle(x+6, y+ch-8, x+cw-6, y+ch-8, fill="#00ff00", outline="")
                # CH label (top center) and value text (center of bar)
                label = self.canvas.create_text(x + cw//2, y + self.cell_header_h//2, text=f"CH{c+1}", fill="#ffffff", font=('TkDefaultFont', 9, 'bold'))
                valtxt = self.canvas.create_text(x + cw//2, y + ch - 18, text="0", fill="#ffffff", font=('TkDefaultFont', 10, 'bold'))
                self.cell_rects[r][c] = (rect, bar)
                self.cell_texts[r][c] = (label, valtxt)
                # bind click
                self.canvas.tag_bind(rect, '<Button-1>', lambda ev, rr=r, cc=c: self.on_cell_click(rr, cc))
                self.canvas.tag_bind(bar, '<Button-1>', lambda ev, rr=r, cc=c: self.on_cell_click(rr, cc))
                self.canvas.tag_bind(label, '<Button-1>', lambda ev, rr=r, cc=c: self.on_cell_click(rr, cc))
                self.canvas.tag_bind(valtxt, '<Button-1>', lambda ev, rr=r, cc=c: self.on_cell_click(rr, cc))

    def on_cell_click(self, r, c):
        # Simple highlight toggle for now
        rect, bar = self.cell_rects[r][c]
        self.canvas.itemconfig(rect, outline="#ffff00")
        # revert after short time
        self.root.after(300, lambda: self.canvas.itemconfig(rect, outline="#555555"))
        print(f"Cell clicked: row={r+1} col={c+1} value={self.cell_values[r][c]}")

    def on_resize(self, event):
        self.create_grid()
        self.update_visuals()

    def toggle_connect(self):
        if self.reader is None:
            port = self.port_var.get()
            if not port:
                messagebox.showerror("Port required", "Please select a serial port first")
                return
            baud = int(self.baud_var.get())
            self.reader = SerialReader(port, baud, block_queue)
            self.reader.start()
            self.connect_btn.config(text="Disconnect")
            self.status_var.set(f"Connected: {port} @ {baud}")
        else:
            self.reader.stop()
            self.reader = None
            self.connect_btn.config(text="Connect")
            self.status_var.set("Disconnected")

    def poll_queue(self):
        try:
            while True:
                item = block_queue.get_nowait()
                tag, data = item
                if tag == "__error__":
                    messagebox.showerror("Serial error", data)
                    if self.reader:
                        self.reader.stop()
                        self.reader = None
                        self.connect_btn.config(text="Connect")
                        self.status_var.set("Disconnected")
                elif tag == "__block__":
                    self.handle_block(data)
        except queue.Empty:
            pass
        self.root.after(50, self.poll_queue)

    def handle_block(self, block_text):
        # parse block into per-mux values
        # reset cell values
        new_vals = [[0]*COLS for _ in range(ROWS)]
        # Process each line
        for line in block_text.splitlines():
            m = RE_MUX.search(line)
            if not m:
                continue
            mux_num = int(m.group(1))
            row_idx = mux_num - 1
            if row_idx < 0 or row_idx >= ROWS:
                continue
            for chm in RE_CHVAL.finditer(line):
                ch_idx = int(chm.group(1))  # 1-based channel index reported by Pico
                val = int(chm.group(2))
                # map channels 1..8 to columns 0..7; ignore others
                if 1 <= ch_idx <= COLS:
                    col_idx = ch_idx - 1
                    new_vals[row_idx][col_idx] = val
        self.cell_values = new_vals
        self.update_visuals()

    def update_visuals(self):
        for r in range(ROWS):
            for c in range(COLS):
                rect, bar = self.cell_rects[r][c]
                label_id, val_id = self.cell_texts[r][c]
                val = self.cell_values[r][c]
                # scale height
                x1, y1, x2, y2 = self.canvas.coords(rect)
                inner_x1 = x1 + 6
                inner_x2 = x2 - 6
                inner_h = (y2 - y1) - 12
                # compute bar top based on value
                try:
                    frac = max(0.0, min(1.0, float(val) / MAX_ADC))
                except Exception:
                    frac = 0.0
                bar_top = y2 - 6 - int(frac * inner_h)
                # update bar coords
                self.canvas.coords(bar, inner_x1, bar_top, inner_x2, y2 - 6)
                # set color (green->yellow->red)
                if frac > 0.66:
                    color = '#ff4444'
                elif frac > 0.33:
                    color = '#ffcc33'
                else:
                    color = '#44ff88'
                self.canvas.itemconfig(bar, fill=color)
                # update value text (keep CH label unchanged)
                self.canvas.itemconfig(val_id, text=str(val))
                # choose text color for value for contrast
                if frac > 0.5:
                    val_color = '#ffffff'
                else:
                    val_color = '#ffffff'
                self.canvas.itemconfig(val_id, fill=val_color)


def main():
    root = tk.Tk()
    app = MuxGridApp(root)
    root.geometry('900x500')
    root.mainloop()


if __name__ == '__main__':
    main()
