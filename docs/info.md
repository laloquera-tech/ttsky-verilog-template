## How it works

This project detects activity in a digitized signal that varies around an offset, such as a conditioned surface EMG signal. It receives unsigned 12-bit samples, subtracts an adjustable offset, takes the absolute value, and averages 64 samples.

MAV means **Mean Absolute Value**:

`MAV = floor(sum(abs(sample[i] - offset), i = 0..63) / 64)`

ACTIVE becomes 1 when MAV exceeds the high threshold and 0 when it falls below the low threshold. Between the thresholds, including equality, it keeps its previous state. A new decision is made every 64 samples. This detects amplitude activity, not specific gestures.

## How to test

1. Select the project on the Tiny Tapeout board and use the ESP32-S3 example in `examples/esp32s3_emg_mav`.
2. Connect the eight input data lines, WRITE, CMD, clock, reset, ACTIVE, and the two toggle outputs as shown in the example README. Tie VIEW and BYTE_SELECT low; the eight result outputs may remain disconnected.
3. Run the default sensor-free test. The controller sends each 12-bit sample as a command and two data bytes, checks the activity response, and then repeats low/high amplitude blocks.
4. For an analog test, connect a conditioned sensor to the controller ADC, adjust the offset and thresholds, and disable demo mode.

The controller keeps sending samples; no read command is needed for ACTIVE. At 2000 samples/s, decisions occur every 32 ms. If samples stop, ACTIVE retains its last state.

See [the full guide](user_guide.md) and [the Arduino example](../examples/esp32s3_emg_mav/README.md) for the protocol and wiring.

## External hardware

- Tiny Tapeout board containing the project, or an FPGA implementation.
- ESP32-S3 development board with enough GPIO; the example targets ESP32-S3-DevKitC-1.
- Optional conditioned surface EMG sensor with a nominal 0–3.3 V output. Keep the actual ADC input within its measurable range; the S3 ADC may saturate before 3.3 V.
- Power and connection wires. Use appropriate isolation for measurements on a person.
