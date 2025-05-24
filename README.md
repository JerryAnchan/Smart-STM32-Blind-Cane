Powered by GitHub Copilot.

# Program Overview

This project is an embedded safety monitoring system based on the STM32 microcontroller. It integrates multiple sensors and modules to provide real-time detection, alerting, and user interaction, making it suitable for elderly care, personal safety, and similar scenarios.

## Key Features

- **Distance Detection**
  - Utilizes an ultrasonic sensor (HC-SR04) to measure the distance to obstacles.
  - Triggers alerts when the measured distance falls below a user-configurable safety threshold.

- **Fall Detection**
  - Employs a 3-axis accelerometer (ADXL345) to detect falls or abnormal tilts.
  - Automatically triggers alarms and sends notifications upon detecting a fall.

- **GPS Positioning**
  - Acquires and processes GPS data to provide real-time location information.
  - Includes error handling for weak or lost GPS signals.

- **Buzzer Alarm**
  - Implements a state machine to control the buzzer for various alert modes (short beep, long beep, emergency alarm, etc.).

- **OLED Display**
  - Displays real-time sensor data, system status, and configuration menus.
  - Supports both English and Chinese character display.

- **SMS Alert via GSM Module**
  - Sends alarm and location information via SMS using a GSM module.
  - Uses a custom protocol (e.g., `config,set,sms,<phone>,<hex_msg>`) with UTF-8 content converted to hexadecimal string format.

- **User Interaction**
  - Allows users to configure safety distance, emergency contact number, and other parameters via physical keys.
  - Supports one-key SOS and cancel functions.

## System Initialization

- Initializes all essential hardware peripherals: GPIO, I2C, UART, timers, sensors, and display.
- Loads configuration parameters (e.g., safety distance, phone number) from non-volatile memory.

## Main Loop

- Continuously monitors sensor data and user input.
- Updates the OLED display with current status and sensor readings.
- Detects abnormal conditions (e.g., fall, unsafe distance) and triggers corresponding alarms.
- Handles SMS sending and buzzer control based on system events.

## Code Structure

- Modular design: Each feature is encapsulated in its own function or module for clarity and maintainability.
- The main loop orchestrates sensor polling, event detection, user interaction, and peripheral control.

---

> **Note:**  
> The system is designed for easy expansion. Additional features such as advanced audio playback (WT588D) or enhanced SIM card communication can be integrated as needed.

---
