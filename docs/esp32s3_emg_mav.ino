#include <Arduino.h>

// ESP32-S3-DevKitC-1 GPIO numbers, not physical header positions.
// Read README.md before connecting a board or a sensor.
constexpr uint8_t DATA_PINS[8] = {5, 6, 7, 8, 9, 10, 11, 12};
constexpr uint8_t WRITE_PIN = 13, CMD_PIN = 14, CLOCK_PIN = 15;
constexpr uint8_t RESET_PIN = 16, ACTIVE_PIN = 17, RESULT_PIN = 18;
constexpr uint8_t ACK_PIN = 21, ADC_PIN = 4;
constexpr bool DEMO_MODE = true;  // Start without a sensor.
constexpr uint16_t OFFSET = 1995, LOW_THRESHOLD = 10, HIGH_THRESHOLD = 16;
constexpr uint32_t SAMPLE_PERIOD_US = 500;  // Best-effort 2000 samples/s.
constexpr uint32_t CLOCK_HALF_US = 2;  // Pulsed clock, NOT continuous 1 MHz.
static_assert(OFFSET <= 4095 && LOW_THRESHOLD < HIGH_THRESHOLD &&
              HIGH_THRESHOLD <= 4095, "Invalid settings");

uint32_t nextSampleUs = 0, samplesSent = 0, missedDeadlines = 0;
uint8_t samplesInBlock = 0;
int lastResult = 0;

void stopWithError(const char *message) {
  digitalWrite(WRITE_PIN, LOW);
  digitalWrite(CLOCK_PIN, LOW);
  Serial.println(message);
  // ACTIVE can retain its last state. This is not a safety controller.
  while (true) delay(1000);
}

void clockCycle() {
  delayMicroseconds(CLOCK_HALF_US);
  digitalWrite(CLOCK_PIN, HIGH);
  delayMicroseconds(CLOCK_HALF_US);
  digitalWrite(CLOCK_PIN, LOW);
  delayMicroseconds(CLOCK_HALF_US);
}

void writeByte(bool command, uint8_t value) {
  digitalWrite(WRITE_PIN, LOW);
  digitalWrite(CMD_PIN, command ? HIGH : LOW);
  for (uint8_t i = 0; i < 8; ++i)
    digitalWrite(DATA_PINS[i], (value >> i) & 1);
  clockCycle();
  const int previousAck = digitalRead(ACK_PIN);
  digitalWrite(WRITE_PIN, HIGH);
  clockCycle();
  const int newAck = digitalRead(ACK_PIN);
  digitalWrite(WRITE_PIN, LOW);
  clockCycle();
  if (newAck == previousAck)
    stopWithError("ERROR: no ACK; check wiring, project selection and clock ownership.");
}

void writeWord(uint8_t command, uint16_t value) {
  if (value > 4095) stopWithError("ERROR: value outside 12-bit range.");
  writeByte(true, command);
  writeByte(false, value & 0xFF);
  writeByte(false, value >> 8);
}

void resetChip() {
  digitalWrite(WRITE_PIN, LOW);
  digitalWrite(CMD_PIN, LOW);
  digitalWrite(RESET_PIN, LOW);
  clockCycle();
  clockCycle();
  digitalWrite(RESET_PIN, HIGH);
  clockCycle();
  samplesInBlock = 0;
  lastResult = digitalRead(RESULT_PIN);
  if (lastResult != 0 || digitalRead(ACTIVE_PIN) != 0 || digitalRead(ACK_PIN) != 0)
    stopWithError("ERROR: unexpected reset outputs.");
}

void configureChip(uint16_t offset, uint16_t low, uint16_t high) {
  if (offset > 4095 || low >= high || high > 4095)
    stopWithError("ERROR: invalid thresholds/offset.");
  writeWord(3, 4095);
  writeWord(2, low);
  writeWord(3, high);
  writeWord(1, offset);
  writeByte(true, 5);
  samplesInBlock = 0;
  lastResult = digitalRead(RESULT_PIN);
}

void sendSample(uint16_t sample) {
  writeWord(0, sample);
  ++samplesInBlock;
  const int result = digitalRead(RESULT_PIN);
  const bool expectedChange = samplesInBlock == 64;
  if ((result != lastResult) != expectedChange)
    stopWithError("ERROR: unexpected result cadence; check data/control wiring.");
  if (expectedChange) samplesInBlock = 0;
  lastResult = result;
}

void selfTest() {
  configureChip(1995, 10, 16);
  const uint16_t values[] = {2003, 2011, 2012, 2011, 2005, 2004};
  const uint8_t expected[] = {0, 0, 1, 1, 1, 0};
  for (uint8_t block = 0; block < 6; ++block) {
    for (uint8_t i = 0; i < 64; ++i) sendSample(values[block]);
    if (digitalRead(ACTIVE_PIN) != expected[block])
      stopWithError("ERROR: activity self-test failed.");
  }
  Serial.println("Activity self-test PASS (six 64-sample blocks).");
}

void setup() {
  Serial.begin(115200);
  for (uint8_t pin : DATA_PINS) {
    digitalWrite(pin, LOW);
    pinMode(pin, OUTPUT);
  }
  const uint8_t controls[] = {WRITE_PIN, CMD_PIN, CLOCK_PIN, RESET_PIN};
  for (uint8_t pin : controls) {
    digitalWrite(pin, LOW);
    pinMode(pin, OUTPUT);
  }
  pinMode(ACTIVE_PIN, INPUT);
  pinMode(RESULT_PIN, INPUT);
  pinMode(ACK_PIN, INPUT);
  pinMode(ADC_PIN, INPUT);
  analogReadResolution(12);
  analogSetPinAttenuation(ADC_PIN, ADC_11db);
  delay(1000);
  resetChip();
  selfTest();
  resetChip();
  configureChip(OFFSET, LOW_THRESHOLD, HIGH_THRESHOLD);
  Serial.println(DEMO_MODE ? "DEMO: synthetic samples, not an EMG recording."
                           : "ADC: raw counts; calibrate offset and thresholds.");
  Serial.println("sample,active,missed_deadlines");
  nextSampleUs = micros() + SAMPLE_PERIOD_US;
}

void loop() {
  const uint32_t now = micros();
  if (static_cast<int32_t>(now - nextSampleUs) < 0) return;
  if (now - nextSampleUs >= SAMPLE_PERIOD_US) {
    ++missedDeadlines;
    nextSampleUs = now;  // No burst of catch-up samples.
  }
  nextSampleUs += SAMPLE_PERIOD_US;
  const int amplitude = ((samplesSent / 2048) & 1) ? 40 : 5;
  const int synthetic = static_cast<int>(OFFSET) +
                        ((samplesSent & 1) ? amplitude : -amplitude);
  if (DEMO_MODE && (synthetic < 0 || synthetic > 4095))
    stopWithError("ERROR: offset leaves insufficient room for demo waveform.");
  const uint16_t sample = DEMO_MODE ? static_cast<uint16_t>(synthetic)
                                  : analogRead(ADC_PIN);
  sendSample(sample);
  ++samplesSent;
  // About four lines/s; skip a line rather than wait for serial buffer space.
  if ((samplesSent % 512) == 0 && Serial.availableForWrite() >= 48)
    Serial.printf("%u,%d,%lu\n", static_cast<unsigned>(sample),
                  digitalRead(ACTIVE_PIN), static_cast<unsigned long>(missedDeadlines));
}
