Powered by GitHub Copilot.

# STM32 Smart Cane (VL53L1X + Air780EPM)

A smart cane based on STM32F103C8 with laser ranging, fall detection, GPS/LBS dual-mode positioning, and GSM SMS alerting.

## Hardware Overview

| Module | Model | Interface | Pins |
|--------|-------|-----------|------|
| MCU | STM32F103C8 | — | — |
| Laser Ranging | VL53L1X | I2C | SDA=PB4, SCL=PB5 |
| Accelerometer | ADXL345 | I2C | SDA=PC14, SCL=PC15 |
| GPS | GPS Module | USART3 | 9600bps |
| GSM | SIM800C | USART1 | 9600bps |
| Display | OLED 128×64 | I2C | SDA=PB6, SCL=PB7 |
| Buzzer | Active Buzzer | GPIO | PC13 |
| Keys | 5-Key | GPIO | PB12~15, PA8 |
| Light/Water Sensor | — | GPIO | PA1 (light), PB9 (water) |

## Features

- **Obstacle Ranging**: VL53L1X with 0–4.5 m range; configurable safety distance threshold
- **Fall Detection**: ADXL345 acceleration magnitude check; confirmed after 2 s of sustained tilt
- **Dual-Mode Positioning**: GPS preferred; automatic fallback to cell tower (LBS) coordinates when GPS has no fix
- **SMS Alert**: Automatic emergency SMS with coordinates on fall detection or manual SOS
- **Real-Time OLED Display**: Distance, latitude/longitude, and settings
- **Key Configuration**: Safety distance threshold and contact phone number (flash-saved, power-off persistent)
- **Voice Interaction**: USART2 communication with voice module (0x31 = SOS, 0x32 = query distance)

## Directory Structure

```
├── CMSIS/              # Cortex-M3 startup files
├── Driver/             # Peripheral drivers + application modules
│   ├── VL53L1x_STM32.c/h   # VL53L1X ranging driver
│   ├── adxl345.c/h          # ADXL345 accelerometer
│   ├── GPS.c/h              # GPS NMEA parser
│   ├── gsm.c/h              # SIM800C SMS / LBS positioning
│   ├── OLED_I2C.c/h         # OLED display driver
│   ├── gpio.c/h             # Buzzer / keys / LED / sensor IO
│   ├── wt588d.c/h           # Buzzer state machine
│   ├── app_ui.c/h           # UI display and key settings
│   ├── app_sensor.c/h       # Sensor processing (ranging / fall / GPS)
│   └── app_utils.c/h        # Flash init / UART buffer clear / data conversion
├── Libraries/          # STM32 Standard Peripheral Library
├── System/             # Delay / system config / UART drivers
├── User/               # Main program + project file
│   ├── main.c               # Main control logic + interrupt handlers
│   └── 程序.uvprojx          # Keil project file
├── Example/            # ST official VL53L1X reference projects (not the main build target)
└── Output/             # Build output (gitignored)
```

## Build and Flash

1. Open `User/程序.uvprojx` in Keil µVision
2. Select Target 1, click Build
3. Flash to the STM32F103C8 board
4. Power on and observe the OLED self-test sequence: I2C Test → VL53L1X Init → GSM Init → LBS Init

## Runtime Flow

1. **Power-On Self-Test**: I2C bus test → VL53L1X device ID check → sensor initialization (halts with diagnostics on failure)
2. **GSM/LBS Init**: SIM800C module ready → acquire cell tower coordinates as GPS fallback
3. **Main Loop** (20 ms cycle):
   - Key scan → UI refresh → GPS parse → fall detection → distance update
   - Distance below threshold → buzzer + voice alert
   - Fall confirmed / manual SOS → automatic emergency SMS with coordinates

## Related Documentation

- [VL53L1X_QuickRef.md](VL53L1X_QuickRef.md) — VL53L1X quick reference
- [VL53L1X_Integration_Notes.md](VL53L1X_Integration_Notes.md) — Integration notes
- [VL53L1X_Troubleshooting.md](VL53L1X_Troubleshooting.md) — Troubleshooting guide
