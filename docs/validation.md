# Validation scope

## Arduino integration checks

- Compiled the actual `.ino` as C++17 with a small host-side Arduino API stub, using warnings as errors.
- Ran its startup self-test and 8192 synthetic streaming samples against an independent software model of the chip's command interface and MAV logic.
- Captured 77,268 rising clock edges from that execution and replayed the input bus, control signals, and reset into the actual Verilog module using Icarus Verilog 11.0.
- Checked ACTIVE, ACK_TOGGLE, RESULT_TOGGLE, and the output-enable mask at every replayed edge. Result: PASS.
- Preserved the RTL without changes. Its Cocotb regression includes arithmetic/protocol checks and the two recorded EMG input files.

The host API stub is a verification aid, not an ESP32 emulator. Its GPIO execution time, ADC values, serial behavior, and interrupt timing do not represent physical hardware. Trace replay checks logical transfers, not analog acquisition, board compatibility, or physical timing margins.

## Still required

- Compile with the Arduino-ESP32 board package and the settings for your actual board. The ESP32 cross-toolchain was not available for this delivery.
- Check the physical Tiny Tapeout carrier connector mapping, project selection, voltage compatibility, and external control of clk/reset/input pins.
- Test startup, analog acquisition, sampling jitter, and signal levels on hardware.
- Calibrate offset and thresholds with your own ADC and sensor.
- Run the GitHub test and GDS workflows for the final repository commit and review the physical-flow results before fabrication.

The sketch is a bring-up example, not validated firmware for medical use or actuator safety.
