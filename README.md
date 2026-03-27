# STM32 Smart Cane Project (VL53L1X + Air780EPM)

This project is based on STM32F10x and targets smart cane scenarios, providing obstacle ranging, fall detection, GPS positioning, and 4G-based alerting.

## Hardware Overview

- MCU: STM32F10x
- Distance Sensor: VL53L1X ToF (I2C, SDA=PB4, SCL=PB5)
- Accelerometer: ADXL345 (fall detection)
- Positioning: GPS module
- 4G Communication Module: Air780EPM (alert/SMS/data channel)
- Display: OLED
- Voice/Keys/Buzzer: local interaction and alerting

## Key Features

- Forward obstacle detection and threshold warning
- Fall detection and emergency alarm
- GPS location reporting
- Air780EPM-based 4G communication and alerts
- Real-time OLED status display
- Key-based parameter configuration (safety distance, contact number, etc.)

## Software Layout

- User/main.c: main control loop, event handling, UI logic
- Driver/VL53L1x_STM32.c: VL53L1X driver and ranging initialization
- Driver/gsm.c: 4G communication flow (project baseline: Air780EPM)
- System/, Libraries/: platform and low-level support

## Build and Flash

1. Open User/程序.uvprojx in Keil.
2. Select the target.
3. Build and flash to the board.
4. Power on and observe OLED/serial output.

## Runtime Notes

- The system performs sensor and communication checks during startup.
- In the main loop, it continuously updates distance, fall state, and GPS data.
- On alarm events, alerts are triggered locally and through Air780EPM.

## Important Notes

- The 4G module in this project is standardized as Air780EPM.
- If a different 4G module is used, update AT commands and initialization in Driver/gsm.c.
- Ranging accuracy can be further tuned with offset/linear calibration in the VL53L1X driver.

## About Example/

The Example/ directory keeps ST official VL53L1X reference projects for behavior comparison and debugging. It is not the main build target of this repository.
