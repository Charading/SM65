Serial GUI for MCP3208 + HC4067 scans
=====================================

This small Python GUI reads scan blocks printed by your Pico (USB CDC) and shows
a 5x8 grid of bar graphs so you can visually see which sensor was pressed.

Quick start
-----------
1. Install pyserial:

   python -m pip install pyserial

2. Run the GUI from this project folder:

   python serial_gui.py

3. Select the COM port (the Pico's USB CDC port) and press Connect.

Notes & assumptions
-------------------
- The GUI expects the Pico to print a full scan as a block of lines terminated by
  a short dashed separator (e.g. "-----------"). The current Pico code prints
  each block then prints a dashed line; this GUI uses that separator.
- The GUI maps lines of the form:

    MUX 1 | CH1: 4096 | CH3: 3972 |

  to a grid where the row is the MUX index (1..5) and the column is CH1..CH8.
  Channels beyond CH8 are ignored by default. You can adapt the parsing logic in
  `serial_gui.py` if your channel mapping differs.

- The grid displays numeric values (0..4095) and colored bars. Click a cell to
  highlight it (this is a placeholder for actuation logic; if you later want to
  send control commands to the Pico, we can add a simple protocol).

- Tweak parameters (thresholds, grid size) directly in `serial_gui.py`.

Integration with QMK / submodule plan
------------------------------------
This GUI is standalone. If you want to make the Pico scanning code into a
reusable submodule for QMK, I can extract the scanning logic into a small
C source + header and provide a CMake-friendly layout. The GUI can remain a
separate host-side tool that listens to the Pico's serial output.

If you want any changes to the layout (different rows/columns, CSV export,
logging to file, or auto-detection of the Pico), tell me and I'll update it.