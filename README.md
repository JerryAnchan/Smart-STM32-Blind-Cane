Powered by GitHub Copilot.

# Smart Blind Cane Based on STM32

This project is a smart blind cane system powered by the STM32F103C8T6 microcontroller. Designed to assist visually impaired individuals, it combines multiple sensors and modules to provide real-time safety monitoring, alerts, and user interaction. The system emphasizes modularity, reliability, and expandability for future enhancements.

## Key Features

- **Ultrasonic Distance Measurement**
  - Detects obstacles ahead with the HC-SR04 ultrasonic sensor.
  - Triggers buzzer and/or SMS alerts if the distance falls below a user-configurable threshold.
  - Distance data is available via serial communication.

- **Fall Detection**
  - Uses the ADXL345 3-axis accelerometer to sense falls or abnormal tilts.
  - Automatically triggers alarms and notifications upon detecting a fall.

- **Water Immersion Detection**
  - Employs a water sensor (connected to PB9) to detect immersion.
  - Activates the buzzer immediately to alert the user.

- **GPS Positioning**
  - Integrates a GPS module for real-time location tracking.
  - Handles signal loss and includes location data in emergency SMS alerts.

- **Buzzer Alarm**
  - Implements a state machine for multiple alert modes (short beep, long beep, emergency alarm, etc.).

- **OLED Display**
  - Shows real-time sensor data, system status, and alert messages.
  - Supports both English and Chinese character display.
  - Displays critical emergency messages such as "Emergency SOS."

- **SMS Alerts via GSM Module**
  - Sends SMS alerts (with location information) using the SIM800 GSM module.
  - Retries SMS sending up to 3 times on failure.
  - Uses a custom protocol, encoding UTF-8 content as hexadecimal strings for compatibility.

- **User Interaction**
  - Users can configure safety distance, emergency contacts, and other parameters via physical keys.
  - Supports one-key SOS and cancel functions.
  - SOS can be triggered by external events or serial commands.

- **Voice Module (ASRPRO) via USART2**
  - Supports voice commands for SOS, distance query, and more.
  - The distance query command (0x32) returns the current distance via USART2.

## System Initialization

- Initializes hardware peripherals: GPIO, I2C, UART (USART2 for ASRPRO), timers, sensors, and display.
- Loads user-configured parameters (like safety distance and emergency contact) from non-volatile memory.

## Main Loop Logic

- Continuously polls sensors and user input.
- Updates OLED display with real-time data and system status.
- Detects abnormal events (falls, unsafe distance, water immersion) and triggers alarms.
- Manages SMS sending, buzzer control, and serial communication based on events.

## Serial Communication

- **USART2**: Connects to the ASRPRO voice recognition module.
  - Receives voice command codes and triggers corresponding functions (e.g., SOS, distance query).
  - Sends current distance via USART2 when command 0x32 is received.
- Other USARTs (such as USART3) may be used for GPS or debugging.
- Serial communications follow standard STM32 UART/USART initialization and data exchange protocols.

## Code Structure

- Modular design: Each function or feature is implemented in its own module for clarity.
- Main loop coordinates sensor polling, event handling, user interaction, and peripheral control.
- Robust error handling and retry mechanisms ensure reliable operation.

---

> **Note:**  
> The system is designed for easy expansion. Additional features such as advanced audio playback (WT588D), improved SIM card communication, or new sensors can be integrated as needed.

---

This project is ideal for multi-level safety monitoring and intelligent user interaction scenarios for the visually impaired. The codebase is clear and modular, making it suitable for further development and customization.

---
