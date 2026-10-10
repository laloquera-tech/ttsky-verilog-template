# Simulation

The Python testbench uses Cocotb to drive the Verilog design. Python sends commands and 12-bit samples through the same interface used by an external controller. It calculates expected results and compares them with the chip outputs.

## Run on GitHub

1. Put the repository files at the root of your GitHub repository. Do not place them inside an extra `EMG_MAV` directory.
2. Commit the changes to your working branch.
3. Open **Actions** and select the test workflow for that commit. If a manual **Run workflow** button is available, choose your branch and run it; otherwise use the run triggered by the commit.
4. Wait for the run to finish. Open the test job to read the result.
5. Open the run summary and download its simulation artifact, normally named `test-results`.
6. Extract the artifact and find `tb.fst`. Open it in Surfer or GTKWave to inspect the waveform. The optional `tb.gtkw` layout is for GTKWave.
7. Run or inspect the GDS workflow for the same commit. Its gate-level simulation checks the synthesized implementation and needs the power connections already present in `tb.v`.

Artifact availability depends on the workflow finishing its upload step. If a run fails, read the first failing job before looking for waveforms.

## Signals to inspect

| Signal | What to look for |
| --- | --- |
| `clk`, `rst_n`, `ena` | Clock, reset, and enable |
| `ui_in[7:0]` | Command or data byte sent to the chip |
| `uio_in[0]` | WRITE: sampled low-to-high transition captures a byte |
| `uio_in[1]` | CMD: 1 for a command, 0 for a data byte |
| `uio_out[5]` | Live activity state |
| `uio_out[6]` | Toggles when a 64-sample MAV result is ready |
| `uio_out[7]` | Toggles when a byte is captured |
| `uo_out[7:0]` | Selected byte from the last snapshot |

In RTL simulation, also expand `tb.user_project` to inspect `centered`, `magnitude`, `mav`, `count`, and `activity`. Internal names may change after synthesis.

The bus transfer clock is 1 MHz in this test. Recorded samples are replayed faster than their original acquisition time: waveform time is the simulation timeline, not the duration of the original recording.

## Checks included

The test checks arithmetic limits, the final sample of each window, strict threshold comparisons and hysteresis, configuration changes, invalid transfers, held WRITE, snapshot behavior, enable behavior, and random sample blocks. It also replays 8,000 samples from each of the two HEX files.

These recordings provide regression examples. They do not establish universal thresholds for every person or sensor.

## Optional local run

A Python environment alone is not enough: this test also needs a Verilog simulator. With Icarus Verilog, Make, and Python installed, run from the `test` directory:

```sh
python -m pip install -r requirements.txt
make
```

Use GitHub Actions if installing the simulator locally is inconvenient. Do not commit generated waveforms, simulation build directories, or test result files as source files.
