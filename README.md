# EMG MAV Activity Detector

This is my Tiny Tapeout project for detecting activity in a single EMG channel. The chip receives digital samples, subtracts an adjustable offset, takes the absolute value, and calculates the mean absolute value (MAV) of every 64 samples. Two adjustable thresholds decide when the activity output turns on or off.

The same idea can be used with other sensors when the size of a signal is useful. The chip needs an external ADC or a microcontroller to convert an analog signal into digital samples.

## Main features

- One channel with unsigned 12-bit samples.
- Adjustable offset and activity thresholds.
- Fixed, non-overlapping windows of 64 samples.
- A live activity output and a toggle for each new MAV result.
- Snapshot readout of the centered sample, absolute value, MAV, and status.
- A simple parallel command interface with an 8-bit input bus.

The intended operating clock is 1 MHz. This is not a measured maximum clock frequency. Sampling speed is set by the external controller. For example, 2,000 accepted samples per second gives one MAV result every 32 ms.

## Files

| File | Purpose |
| --- | --- |
| `src/emg_mav.v` | Chip logic: `tt_um_francodelgado_emg_mav` |
| `info.yaml` | Project information and logical pin names |
| `docs/info.md` | How the chip works, how to test it, and external hardware |
| `test/tb.v` | Verilog simulation wrapper, including gate-level power connections |
| `test/test.py` | Cocotb checks and recorded-sample replay |
| `test/rest_ch0.hex` | Rest recording used by the test |
| `test/hand_close_ch0.hex` | Hand-close recording used by the test |
| `test/tb.gtkw` | Optional GTKWave signal layout |
| `test/Makefile` | RTL and gate-level simulation setup |

Start with [the project guide](docs/info.md). For simulation, read [the test instructions](test/README.md).

## Hardware for a first test

Use a Tiny Tapeout development board or an FPGA implementation, an ESP32-S3 development board with enough available GPIO, and a suitable analog input source. An EMG test also needs a conditioned EMG sensor signal. The complete parallel interface needs more pins than a small XIAO board conveniently exposes; check the exact ESP32-S3 board before wiring it.

The chip does not include an ADC, analog amplification, or permanent configuration storage. Settings return to their defaults after reset. ESP32-S3 firmware and a board-specific wiring diagram are not included in this repository.

## Validation

The Cocotb test covers arithmetic, thresholds, protocol behavior, and replay of two recorded sample files. Passing simulation does not by itself mean the project is ready for fabrication. Run the test and GDS workflows on the final commit, and check the physical-flow results and the submission requirements.
