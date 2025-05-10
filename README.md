Powered by GitHub Copilot.

# Program Overview

This program is built for an embedded system using the STM32 platform, offering features for enhanced safety and monitoring. It integrates various sensors and modules to provide real-time feedback and alerts. Below are the key functionalities:

## Features

1. **Distance Detection**: 
   - Measures distance using an ultrasonic sensor (`HC_SR04`).
   - Supports configurable safety distance settings and triggers alerts when breached.

2. **Fall Detection**:
   - Employs an accelerometer (`adxl345`) to detect falls or significant tilts.
   - Provides emergency feedback and triggers responses during a fall.

3. **GPS Positioning**:
   - Processes GPS data to display longitude and latitude.
   - Includes error handling for GPS signal issues.

4. **Light Sensing and LED Control**:
   - Automatically lights up an LED based on light sensing, enhancing safety in low-light conditions.

5. **User Interaction**:
   - Features a user interface through an OLED display.
   - Allows users to configure settings (e.g., safety distance) using key inputs.

## System Initialization

- Initializes key hardware components like GPIO, I2C, UART, timers, and sensors.
- Ensures proper configuration of the system on startup.

## Continuous Monitoring

- The main program loop continuously:
  - Monitors key settings.
  - Updates the OLED display.
  - Processes distance and GPS data.
  - Detects falls and triggers appropriate responses.

## Program Structure

- Modular functions for each feature ensure clarity and maintainability.
- Core functionality is implemented in the `main` loop, which handles sensor data processing and user interaction.

Later, functions such as pushing alarm information through SIM card and playing audio through WT288D can be added.
