# Full user guide

For a working controller example and proposed GPIO mapping, see [the ESP32-S3 Arduino example](../examples/esp32s3_emg_mav/README.md). It uses a slower, controller-generated pulsed clock, not a continuous 1 MHz clock. The sampling target remains 2000 samples/s.

## How it works

This project is a small digital activity detector. I designed it mainly for EMG signals, but it can also work with other digitized sensor signals when the offset and thresholds are adjusted.

The circuit receives one channel of unsigned 12-bit samples, from 0 to 4095. It does not include an amplifier or an ADC. A controller sends the samples through an 8-bit parallel interface.

For each sample, the circuit:

1. Subtracts an adjustable offset to center the signal.
2. Takes the absolute value to rectify it.
3. Adds 64 rectified samples and divides the total by 64 to calculate the mean absolute value (MAV). The fractional part is discarded.
4. Compares the MAV with two thresholds to update the activity output.

| MAV value | ACTIVE output |
|---|---|
| Above the high threshold | 1: active |
| Below the low threshold | 0: relaxed / low activity |
| Between the thresholds, including equality | Keep the previous state |

The two thresholds provide hysteresis, which helps avoid repeated switching near a single threshold. The circuit detects amplitude, not a specific hand gesture.

The blocks contain 64 samples and do not overlap. At 2000 samples per second, a new MAV is produced every 32 ms. The intended chip clock is 1 MHz; this is different from the sampling rate and is not a claim about the maximum silicon frequency. The external controller sets the sample rate.

After reset, the offset is **1995**, the low threshold is **10**, and the high threshold is **16**. These are ADC counts, not volts. They are demo settings for one recorded session and should be adjusted for a different sensor or recording.

ACTIVE stays at its last value between decisions. It also stays at its last value if samples stop arriving. Settings return to their defaults after reset; they are not stored permanently.

## How to test

### Start with known numbers

A sensor is not needed for the first test. A controller can send fixed sample values and check the output.

1. Select the project on the Tiny Tapeout board so that `ena=1`.
2. Supply a clock on `clk`. Keep `rst_n=0` for at least two rising clock edges, then release it to 1 while the clock is low.
3. Keep `WRITE=0` between writes. If only ACTIVE is needed, set `VIEW=00` and `BYTE_SELECT=0`.
4. Send samples using the byte protocol below.
5. Observe `uio[5]`, the ACTIVE output.

For a simple connection, let the controller generate the chip clock as well as the data. Set inputs while the clock is low and keep them stable around its rising edge. This interface expects synchronous inputs; an independently clocked controller needs additional timing/clock-domain handling.

### Pin functions

`ui[7:0]` means eight pins carrying one byte. It does not mean eight sensor channels. Pin numbers below are logical project signals, not physical connector positions.

| Pin or group | Direction at the chip | Function |
|---|---|---|
| ui[7:0] | Input | Command or data byte |
| uo[7:0] | Output | Selected result byte |
| uio[0] — WRITE | Input | Capture a new write |
| uio[1] — CMD | Input | 1 for a command, 0 for data |
| uio[2] — VIEW[0] | Input | Result selection, low bit |
| uio[3] — VIEW[1] | Input | Result selection, high bit |
| uio[4] — BYTE_SELECT | Input | 0 for the low byte, 1 for the high byte |
| uio[5] — ACTIVE | Output | Current activity state |
| uio[6] — RESULT_TOGGLE | Output | Changes level after each complete block |
| uio[7] — ACK_TOGGLE | Output | Changes level after each captured write |
| clk | Input | Clock, rising-edge capture |
| rst_n | Input | Synchronous reset, active low |
| ena | Infrastructure input | Project enable |

The eight uio pins have fixed directions in this design: uio[4:0] are inputs and uio[7:5] are outputs. The internal `uio_oe` value is 0xE0. It is not an additional external bus.

### Send a sample

Each sample needs three writes: command 0, low byte, then high byte.

For each write:

1. With the clock low, set `ui`, set `CMD`, and set `WRITE=0`. Generate one clock cycle.
2. With the clock low, set `WRITE=1`. Generate one cycle to capture the byte.
3. With the clock low, set `WRITE=0`. Generate one cycle before the next write.

Keeping WRITE high does not repeatedly capture data. ACK_TOGGLE confirms a captured write, but does not guarantee that the command or data were valid.

For example, sample **2003** is sent as follows. Each row is a separate write:

| CMD | ui, decimal | Meaning |
|---:|---:|---|
| 1 | 0 | Sample command |
| 0 | 211 | Low byte |
| 0 | 7 | High byte |

The chip rebuilds the number as `211 + 256 × 7 = 2003`. With offset 1995, its magnitude is 8. In general, `low_byte = value & 255` and `high_byte = value >> 8`. The high byte must be between 0 and 15.

After reset, send the following blocks in order without resetting between them:

| Sample, repeated 64 times | Expected MAV | Expected ACTIVE |
|---:|---:|---:|
| 2003 | 8 | 0 |
| 2011 | 16 | 0 |
| 2012 | 17 | 1 |
| 2011 | 16 | 1 |
| 2005 | 10 | 1 |
| 2004 | 9 | 0 |

This checks both MAV calculation and hysteresis.

### Change the settings

Commands are sent with CMD=1. Any following data bytes use CMD=0.

| Command, decimal | Action | Following data |
|---:|---|---|
| 0 | Send a sample | Low byte, high byte |
| 1 | Set offset | Low byte, high byte |
| 2 | Set low threshold | Low byte, high byte |
| 3 | Set high threshold | Low byte, high byte |
| 4 | Capture a result snapshot | None |
| 5 | Clear the error flag | None |

Stop sending samples while changing settings. The low threshold must be smaller than the high threshold. To change both in either direction, first set the high threshold to 4095, then set the desired low threshold, then the desired high threshold. Set the offset and resume sampling.

Each valid setting update clears the partial block, sets ACTIVE to 0, and clears the result-valid flag. The old numeric MAV may remain, so wait for 64 new samples before using the next result. Invalid writes set an error flag. A new command cancels an incomplete data word.

Estimate the offset and compare resting and active MAV values before choosing thresholds. The chip does not calibrate itself automatically.

### Read a result

The uo bus can show the stages of processing. Reading them does not stop activity detection.

1. Send command **4** to take a snapshot of the current results.
2. Set VIEW using uio[3:2].
3. Set BYTE_SELECT=0, allow the outputs to settle, and read the low byte on uo[7:0].
4. Set BYTE_SELECT=1, allow the outputs to settle, and read the high byte.
5. Combine them as `low_byte + 256 × high_byte`.

| VIEW = uio[3:2] | Snapshot value |
|---|---|
| 00 | Last centered sample |
| 01 | Last rectified magnitude |
| 10 | Last MAV |
| 11 | Status bits: bit 0 activity, bit 1 result valid, bit 2 error |

Interpret the centered sample as signed 16-bit data: if the combined value is 32768 or more, subtract 65536. MAV and magnitude are unsigned.

The snapshot stays unchanged until another command 4. The direct ACTIVE pin always shows the current decision. RESULT_TOGGLE changes level for every new MAV; it is not a short pulse. Read it often enough to avoid missing changes.

### Run the simulation

Run the **test** workflow in GitHub Actions. When it finishes, download **test-results**, extract **tb.fst**, and open it in Surfer. Add `centered`, `magnitude`, `mav`, and `activity` from `tb.user_project`.

The test includes known inputs, protocol checks, random samples, and two recorded EMG files: **rest_ch0.hex** and **hand_close_ch0.hex**, each containing 8000 samples. The simulation sends samples faster than the original recording rate, so the waveform time axis is not the original acquisition time.

The **gds** workflow also runs **gl_test** to check the gate-level circuit. Run both workflows again after uploading this renamed version.

## External hardware

For a physical test:

- A Tiny Tapeout board containing this project, or an FPGA with the RTL.
- An **ESP32-S3 development board with enough available GPIO pins** to send samples and read the outputs. This guide does not assume a XIAO board.
- An ADC, either inside the controller or external, with a configured 12-bit sample format.
- Wires, a suitable power supply, and a common ground between the digital boards.
- Optionally, a logic analyzer to observe the bus and ACTIVE output.

The basic connection needs **13 digital controller GPIOs** if the controller drives clk and reset and reads only ACTIVE: eight data outputs, WRITE, CMD, clk, rst_n, and one ACTIVE input. Add an ADC input if the same controller acquires the analog signal. VIEW and BYTE_SELECT can be tied low. The included Arduino example uses **15 digital GPIO plus one ADC input**, adding ACK_TOGGLE and RESULT_TOGGLE for transfer checks. Check the exact development board pinout and avoid pins reserved for flash, PSRAM, USB, or other board functions. Physical connector positions and external-control setup depend on the Tiny Tapeout carrier.

For **EMG**, add an EMG acquisition module with amplification, biasing, and suitable signal conditioning. Connect its analog output to the ADC, not to the chip's digital pins. Use an acquisition setup with appropriate power and isolation when attaching electrodes to a person.

For an easier first analog test, use a **potentiometer** within the ADC input range. A conditioned microphone signal or analog vibration sensor can also be used after adjusting the settings and sample rate. For DC sensors, MAV measures the average distance from the selected offset rather than the physical quantity directly.

An SPI or I2C ADC needs a controller or adapter: this chip uses its own parallel byte protocol. Follow the voltage limits of the ADC and Tiny Tapeout board. ACTIVE is a logic signal and cannot power a motor directly.

Possible uses include basic muscle-activity detection, sound or vibration activity detection, amplitude measurement for another controller, and classroom experiments with digital rectification, averaging, and hysteresis.
