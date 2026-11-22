# Bench Results

Board: NUCLEO-F303K8. Input was reported as 12 V during the later tests. The load was reported as two series-connected 0.47 Ω, 50 W resistors (nominal 0.94 Ω total); a later measurement described the load as 1 Ω. Unless a row says otherwise, values below are user-reported DMM readings. A single meter was used sequentially for voltage and current, so derived power is approximate.

## Open-loop measurements

| Duty | Output voltage | Output current | Notes |
|---:|---:|---:|---|
| 10% | 1.34–1.37 V | 1.15 A at one point | Early measurements; current ADC readings 2256 unloaded and 2200 at 1.15 A were used for a provisional display calibration. |
| 15% | 1.88–1.89 V | — | PA0 divider measured around 0.188 V. |
| 24% | 2.81 V | 2.25 A | Sequential readings. |
| 38% | 4.20–4.22 V | 3.70–4.60 A | Current readings differed between sessions/load conditions. |
| 40% | 4.40 V | 3.90 A | Sequential readings. |
| 42% | 4.61 V | 4.02 A | Sequential readings. |
| 44% | 4.81 V | 4.20 A | Sequential readings. |
| 46% | 5.00 V | 4.37 A | Sequential readings. |
| 48% | — | 4.56 A | Direct-load connection reportedly shut the circuit down; cause was not isolated. |
| 30% | 3.40 V | — | 12 V input and nominal 1 Ω load reported. |
| 29% | 3.31–3.32 V | — | LCD showed 3.38 V before voltage-scale calibration. |

## Closed-loop voltage tests

| Target | LCD | DMM | Duty ceiling | Software trip |
|---:|---:|---:|---:|---:|
| 1.00 V | 1.00 V | 1.03 V | 12% | 1.50 V |
| 2.00 V | 2.00 V | 2.02 V | 22% | 2.50 V |
| 3.00 V | 3.00 V | 3.01–3.02 V | 32% | 3.50 V |
| 4.00 V | 4.00 V | 4.00–4.01 V | 40% | 4.50 V |
| 5.00 V | 5.00 V | 4.97–4.98 V | 46% | 5.50 V |

At the 5 V target, output current was later reported around 4.29 A. Since voltage and current were measured in separate meter setups, 21.3 W is only an approximate product of the reported values.

## Measurement caveats

- Current sensing on PA3/ADC1_IN4 was moved from the previous PA4/ADC2_IN1 configuration after the pin mismatch was identified.
- ADC1 and ADC2 calibration was added earlier; the final code uses ADC1 channel switching for PA0 and PA3 and a long sampling time.
- Reported PA3 voltage and ADC readings did not consistently agree. The LCD current calculation is therefore provisional (zero 2256 counts, 56 counts/A from one 1.15 A measurement) and is not suitable for current limiting.
- The 5 A current rating mentioned for the converter is not enforced in firmware. Use external current measurement and observe the load/power ratings during tests.
- The voltage display scale is 10.3136 with nominal 3.3 V ADC reference, calibrated against DMM readings. Validate again if the divider, reference, or board changes.
