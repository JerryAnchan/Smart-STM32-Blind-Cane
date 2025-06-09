Powered by GitHub Copilot.

# Program Overview

This project is an embedded safety monitoring system based on the STM32 microcontroller. It integrates multiple sensors and modules to provide real-time detection, alerting, and user interaction, making it suitable for elderly care, personal safety, and similar scenarios.

## Key Features

- **Distance Detection**
  - Utilizes an ultrasonic sensor (HC-SR04) to measure the distance to obstacles.
  - Triggers alerts when the measured distance falls below a user-configurable safety threshold.
  - Distance information can be queried and transmitted via serial port.

- **Fall Detection**
  - Employs a 3-axis accelerometer (ADXL345) to detect falls or abnormal tilts.
  - Automatically triggers alarms and sends notifications upon detecting a fall.

- **Water Immersion Detection**
  - Uses a water sensor connected to PB9.
  - When water is detected, the buzzer is immediately activated for alerting.

- **GPS Positioning**
  - Acquires and processes GPS data to provide real-time location information.
  - Includes error handling for weak or lost GPS signals.
  - Location information is included in SMS alerts.

- **Buzzer Alarm**
  - Implements a state machine to control the buzzer for various alert modes (short beep, long beep, emergency alarm, etc.).

- **OLED Display**
  - Displays real-time sensor data, system status, and alarm messages.
  - Supports both English and Chinese character display.
  - Shows emergency messages such as "Emergency SOS" when triggered.

- **SMS Alert via GSM Module**
  - Sends alarm and location information via SMS using a SIM800 GSM module.
  - Supports automatic resend if SMS sending fails (up to 3 retries).
  - Uses a custom protocol (e.g., `config,set,sms,<phone>,<hex_msg>`) with UTF-8 content converted to hexadecimal string format.

- **User Interaction**
  - Allows users to configure safety distance, emergency contact number, and other parameters via physical keys.
  - Supports one-key SOS (KEY4) and cancel functions.
  - Emergency SOS can also be triggered by external events or serial commands.

- **Voice Module (ASRPRO) via USART2**
  - USART2 is connected to the ASRPRO voice recognition module.
  - Supports voice commands for SOS, distance query, and other functions.
  - Distance query command (0x32) will trigger the system to send the current distance value back via USART2.

## System Initialization

- Initializes all essential hardware peripherals: GPIO, I2C, UART (including USART2 for ASRPRO), timers, sensors, and display.
- Loads configuration parameters (e.g., safety distance, phone number) from non-volatile memory.

## Main Loop

- Continuously monitors sensor data and user input.
- Updates the OLED display with current status and sensor readings.
- Detects abnormal conditions (e.g., fall, unsafe distance, water immersion) and triggers corresponding alarms.
- Handles SMS sending, buzzer control, and serial communication based on system events.

## Serial Communication

- **USART2** is dedicated to the ASRPRO voice module.
  - Receives voice command codes and responds accordingly (e.g., triggers SOS, returns distance).
  - When 0x32 is received, the system sends the current distance as a string via USART2.
- Other USARTs (such as USART3) can be used for GPS or debugging.

## Code Structure

- Modular design: Each feature is encapsulated in its own function or module for clarity and maintainability.
- The main loop orchestrates sensor polling, event detection, user interaction, and peripheral control.
- Error handling and retry mechanisms are implemented for robust operation.

---

> **Note:**  
> The system is designed for easy expansion. Additional features such as advanced audio playback (WT588D), enhanced SIM card communication, or new sensors can be integrated as needed.

---
