# ESP32-S3 Arduino example

This example sends 12-bit samples to the MAV chip and reads ACTIVE. The ESP32 only acquires and transfers samples; the chip calculates MAV and decides activity. Start with the built-in sensor-free test before connecting an analog source.

## Reference board and wiring

The proposed mapping is for **ESP32-S3-DevKitC-1**, not XIAO ESP32-S3. Numbers below are GPIO numbers printed on the board, not header positions. Check a different board's schematic before using this mapping.

| ESP32-S3 GPIO | Connect to | Direction at ESP32 |
| --- | --- | --- |
| 4 | Conditioned analog signal | ADC input |
| 5 | Chip ui[0] | Output |
| 6 | Chip ui[1] | Output |
| 7 | Chip ui[2] | Output |
| 8 | Chip ui[3] | Output |
| 9 | Chip ui[4] | Output |
| 10 | Chip ui[5] | Output |
| 11 | Chip ui[6] | Output |
| 12 | Chip ui[7] | Output |
| 13 | Chip uio[0], WRITE | Output |
| 14 | Chip uio[1], CMD | Output |
| 15 | Chip clk | Output |
| 16 | Chip rst_n | Output |
| 17 | Chip uio[5], ACTIVE | Input |
| 18 | Chip uio[6], RESULT_TOGGLE | Input |
| 21 | Chip uio[7], ACK_TOGGLE | Input |
| GND | Digital board ground and analog source reference | Ground |

Remaining connections:

- Tie chip uio[2], uio[3], and uio[4] to GND. These are VIEW and BYTE_SELECT.
- Leave chip uo[7:0] disconnected for this example: they are outputs.
- Select the project on the Tiny Tapeout board; its infrastructure must provide ena=1. Do not assume ena is a user header pin.
- Use 10 kOhm pull-downs on chip ui[7:0], WRITE, CMD, clk, and rst_n so these inputs stay defined while the ESP32 boots. Direct GPIO drives take over afterward. Check existing board pulls before adding resistors.
- In demo mode the ADC pin is not read; it may be tied to GND with the sensor disconnected. Remove that tie before connecting a sensor.

This uses 15 digital GPIO plus one ADC input. The strict minimum is 13 digital GPIO, but this sketch also reads both toggle outputs to check transfers and results. Do not omit those two connections without modifying the code.

## Board ownership and electrical checks

This table names logical project signals, not the physical connector positions of a particular Tiny Tapeout carrier. Verify the carrier pinout separately. The ESP32 must be the **only driver** of the listed input signals, including clk and rst_n. Configure the carrier for external control and disable or tri-state its competing controller/clock outputs before connecting. Do not connect two push-pull outputs together. This example does not automate project selection or carrier setup.

Use only voltage-compatible interfaces. ESP32 GPIO uses 3.3 V logic; a bare 1.8 V chip needs appropriate level translation. Power each board as intended, connect grounds, and do not blindly join supply rails from two powered boards. Do not power a bare ASIC directly from this pin table. Never feed a signal into an unpowered board.

## Arduino IDE

1. Install the **esp32 by Espressif Systems** board package in Boards Manager.
2. Open `esp32s3_emg_mav.ino` inside its matching folder.
3. Select **ESP32S3 Dev Module**, or the exact supported board entry for your board. Set flash/PSRAM options to match your module. Select the connected serial port.
4. Leave `DEMO_MODE = true`, upload, and open Serial Monitor at **115200 baud**. If using native USB instead of the USB-to-UART connector, configure USB CDC on boot for that connection.
5. After selecting the chip project and completing the wiring, reset the ESP32 to restart the test. An error stops the sketch until reset.

Expected startup output:

```text
Activity self-test PASS (six 64-sample blocks).
DEMO: synthetic samples, not an EMG recording.
sample,active,missed_deadlines
```

Startup checks six blocks with MAV values 8, 16, 17, 16, 10, and 9. Expected activity is 0, 0, 1, 1, 1, 0. Then the sketch alternates synthetic low/high amplitude every 2048 samples, about 1.024 seconds at the target rate. ACTIVE changes after a completed 64-sample block. These samples test the interface; they are not recorded EMG.

## Switch to a sensor

1. First verify the sensor-free test.
2. Use a properly conditioned analog signal on GPIO4. For initial testing, use a potentiometer or signal generator within the ADC range instead of electrodes.
3. Change `DEMO_MODE` to `false`.
4. Adjust `OFFSET`, `LOW_THRESHOLD`, and `HIGH_THRESHOLD` in ADC counts and upload again. The defaults 1995/10/16 are examples, not calibration for your hardware.
5. Check the `sample` column and measure resting/active MAV using your recorded data to choose thresholds. The sparse serial output is for monitoring, not a full-rate recording or calibration tool.

The code uses `analogReadResolution(12)` and `analogSetPinAttenuation(4, ADC_11db)`. Raw values span 0..4095, but that does not mean a calibrated 0..3.3 V transfer. The documented S3 measurable range at this attenuation is approximately 0..3.1 V. Avoid saturation; if the sensor can swing to 3.3 V, provide suitable analog scaling/buffering and remeasure the offset and thresholds. Do not apply negative voltages or voltages above the board's limits.

For body-connected EMG, use a properly isolated acquisition setup. A generic sensor board connected to a USB-powered computer is not automatically safe for electrodes. This is an educational activity detector, not a medical or safety device.

## Timing and transfer checks

The ESP32 produces clock pulses only while communicating. It keeps data and control stable before each rising edge. This avoids using an unrelated free-running chip clock; **do not enable another clock source**. The software-generated clock is slower than 1 MHz and depends on GPIO call overhead and interrupts. The RTL can hold its state while the clock is stopped; confirm your physical carrier supports this mode.

The target sample period is 500 us (2000 samples/s), giving a 32 ms MAV window if that rate is maintained. This is a best-effort Arduino loop, not precision acquisition. `missed_deadlines` counts occasions at least one full period late, not every small timing error or every lost sample. Check timing with a logic analyzer; avoid enabling Wi-Fi or adding slow work. Interrupts and serial I/O can add jitter. For research-quality acquisition, use timed ADC acquisition with buffering in a later version.

Each sample requires three writes: command 0, low byte, high byte. ACK must toggle on every write, and RESULT_TOGGLE must change every 64 samples. ACK acknowledges capture, not command validity. The startup activity test catches several common wiring/protocol errors but does not prove every data line or analog measurement correct.

If acquisition stops or the sketch reports an error, ACTIVE may remain high. There is no timeout in the chip. Do not connect ACTIVE directly to a motor or safety-critical actuator.

## Validation status

The delivery includes host-side protocol checks and the existing RTL test. These do not replace compilation using your installed Arduino-ESP32 board package or testing on an ESP32-S3 and the physical chip. See `docs/validation.md` for the exact checks performed.
