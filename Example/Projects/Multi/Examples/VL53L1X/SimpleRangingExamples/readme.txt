VL53L1X SimpleRangingExamples (Repository Adaptation Notes)
============================================================

This directory comes from ST official VL53L1X sample projects and is kept as a reference for ranging initialization and measurement flow.

Relationship to the main project:
- This folder is reference code and is not part of the default main build.
- The main project is in the repository root, with core logic under User/ and Driver/.

Hardware baseline in this repository:
- MCU: STM32F10x
- ToF: VL53L1X
- 4G module: Air780EPM

Notes:
- If you validate VL53L1X behavior in this sample, migrate proven changes back to the main driver.
- 4G functionality in this repository follows the Air780EPM implementation in Driver/gsm.c.
