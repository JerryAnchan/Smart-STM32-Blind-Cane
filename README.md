Powered by GitHub Copilot.

# Smart Blind Cane Based on STM32

This project is a smart blind cane system based on the STM32 microcontroller. It integrates multiple sensors and modules to provide real-time safety monitoring, alerts, and user interaction, making it highly suitable for visually impaired users and elderly care.

## Key Features

- **Ultrasonic Distance Measurement**
  - Uses an HC-SR04 ultrasonic sensor to detect obstacles ahead.
  - Triggers alerts (buzzer and/or SMS) when the measured distance falls below a user-configurable safety threshold.
  - Distance data can be queried and transmitted via serial port.

- **Fall Detection**
  - Utilizes an ADXL345 3-axis accelerometer to detect falls or abnormal tilts.
  - Automatically triggers alarms and sends notifications when a fall is detected.

- **Water Immersion Detection**
  - Employs a water sensor connected to PB9.
  - The buzzer is activated immediately upon water detection to alert the user.

- **GPS Positioning**
  - Integrates a GPS module for real-time location tracking.
  - Provides error handling for weak or lost GPS signals.
  - Location information is included in SMS alerts for emergencies.

- **Buzzer Alarm**
  - Implements a state machine for various alert modes (short beep, long beep, emergency alarm, etc.).

- **OLED Display**
  - Shows real-time sensor data, system status, and alert messages.
  - Supports both English and Chinese character display.
  - Displays emergency messages, such as "Emergency SOS".

- **SMS Alert via GSM Module**
  - Sends alerts and location information through SMS using a SIM800 GSM module.
  - Supports automatic resend if SMS fails (up to 3 retries).
  - Uses a custom protocol with UTF-8 content converted to hexadecimal string format for compatibility.

- **User Interaction**
  - Allows users to configure safety distance, emergency contact numbers, and other parameters via physical keys.
  - Supports one-key SOS (KEY4) and cancel functions.
  - SOS can also be triggered by external events or serial commands.

- **Voice Module (ASRPRO) via USART2**
  - USART2 is connected to the ASRPRO voice recognition module.
  - Supports voice commands for SOS, distance query, and more.
  - Distance query command (0x32) triggers the system to send the current distance value via USART2.

## System Initialization

- Initializes all essential hardware peripherals, including GPIO, I2C, UART (USART2 for ASRPRO), timers, sensors, and display.
- Loads configuration parameters (such as safety distance and phone number) from non-volatile memory.

## Main Loop Logic

- Continuously polls sensors and user input.
- Updates the OLED display with current status and sensor readings.
- Detects abnormal conditions (fall, unsafe distance, water immersion) and triggers corresponding alarms.
- Handles SMS sending, buzzer control, and serial communication based on system events.

## Serial Communication Implementation

- **USART2** is dedicated to connecting the ASRPRO voice module:
  - Receives voice command codes and responds accordingly (e.g., triggers SOS, returns distance).
  - When code 0x32 is received, the system sends the current distance as a string via USART2.
- Other USARTs (such as USART3) can be used for GPS data input or debugging purposes.
- All serial communications follow the standard STM32 UART/USART driver procedures for initialization, data reception, and transmission.

## Code Structure

- Modular design: Each feature is encapsulated in its own function or module for clarity and maintainability.
- The main loop orchestrates sensor polling, event detection, user interaction, and peripheral control.
- Error handling and retry mechanisms are implemented for robust operation and reliability.

---

> **Note:**  
> The system is designed for easy expansion. Additional features, such as advanced audio playback (WT588D), enhanced SIM card communication, or new sensors, can be integrated as needed.

---

This project is ideal for scenarios requiring multi-level safety monitoring and intelligent user interaction. The codebase is clear and modular, making it suitable for further development and customization.
