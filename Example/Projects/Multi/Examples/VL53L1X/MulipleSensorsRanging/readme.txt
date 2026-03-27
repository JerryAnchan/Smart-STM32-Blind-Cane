VL53L1X MulipleSensorsRanging (Repository Adaptation Notes)
============================================================

This directory is based on ST's official multi-sensor ranging sample and is used to understand multi-node VL53L1X management.

Relationship to the main project:
- This folder is a reference project and is not the default build target.
- The main project focuses on single VL53L1X integration with smart-cane application logic.

Hardware baseline in this repository:
- MCU: STM32F10x
- ToF: VL53L1X
- 4G module: Air780EPM

Notes:
- If multi-ToF expansion is needed later, use this sample as architecture reference and port into the main project.
- External communication capabilities are unified around Air780EPM adaptation.
