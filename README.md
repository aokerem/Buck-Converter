# Buck Converter Firmware (STM32F303K8)

Firmware for the NUCLEO-F303K8 based buck converter in this project. The current bench-tested build uses closed-loop voltage control with a 5.00 V target.

## Current firmware configuration

- MCU/board: STM32F303K8 / NUCLEO-F303K8
- PWM: TIM1 CH1 on PA8, 40 kHz
- Voltage sensing: PA0 / ADC1 channel 1
- Current sensing: PA3 / ADC1 channel 4
- Control: PI loop, TIM2 interrupt at 1 kHz
- Target: 5.00 V, ramped at 2 V/s
- Duty clamp: 0–46%
- Software overvoltage trip: 5.50 V; fault disables TIM1 main output and latches until reset
- LCD refresh: 500 ms; voltage and current displayed to two decimal places

The voltage scale uses 10.3136 and a nominal 3.3 V ADC reference. This was calibrated against a DMM at several points from 1 V to 5 V. The most recent report at a 5 V target was LCD 5.00 V and DMM 4.97–4.98 V.

## Build

Open `20230319_buckconverter.ioc` / `.project` with STM32CubeIDE and build the Debug configuration. Generated `Debug/` and `Release/` outputs are ignored by Git.

## Bench validation

Reported closed-loop measurements are in [docs/bench-results.md](docs/bench-results.md). These are measurements from the described bench setup, not guarantees for other input voltages, loads, component tolerances, or thermal conditions.

## Important test limitation

The ACS712 current ADC path has not been validated against the measured PA3 voltage. The LCD current value uses a provisional one-point calibration and must not be used as a protection limit. Monitor current with an external meter during bench tests. The firmware's overvoltage trip is also software-based and is not a substitute for hardware protection.
