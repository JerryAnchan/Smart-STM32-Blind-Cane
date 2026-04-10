# 智能导盲杖 · 软件总体设计

> 目标平台：STM32F103（72 MHz Cortex-M3）  
> 开发环境：Keil MDK · 标准外设库（StdPeriph）

---

## 1. 软件分层架构图

```mermaid
graph TB
    subgraph HW["🔌 硬件层 Hardware Layer"]
        direction LR
        HW1[VL53L1X\n激光测距]
        HW2[ADXL345\n三轴加速度计]
        HW3[GPS模块\nUSART3]
        HW4[SIM800C\nGSM / LBS]
        HW5[OLED\n128×64]
        HW6[蜂鸣器\nPC13]
        HW7[5个按键\nPB12~15 PA8]
        HW8[光敏/水位\n传感器]
        HW9[WT588D\n语音模块 USART2]
    end

    subgraph DRV["⚙️ 驱动层 Driver Layer"]
        direction LR
        DRV1[VL53L1x_STM32\nI2C 测距驱动]
        DRV2[adxl345\nI2C 加速度驱动]
        DRV3[GPS\nNMEA 解析]
        DRV4[gsm\n短信 + LBS 定位]
        DRV5[OLED_I2C\n字符/汉字显示]
        DRV6[wt588d\n蜂鸣器状态机]
        DRV7[gpio\nIO 初始化]
        DRV8[iic\n软件 I2C 总线]
        DRV9[timer\nTIM2 / TIM3]
        DRV10[stmflash\nFlash 读写]
        DRV11[usart1/2/3\n串口驱动]
    end

    subgraph APP["📦 应用层 Application Layer"]
        direction LR
        APP1[app_sensor\n传感器业务逻辑\nFallDetection\nGet_Distance\nGet_GPS]
        APP2[app_ui\n界面与按键\nShowHomePage\nKeySettings\nDisplaySetValue]
        APP3[app_utils\n系统工具\nCheckNewMcu\nUsartRx1BufClear]
    end

    subgraph MAIN["🎯 主控层 Main"]
        MAIN1[main.c\n系统初始化\n主循环调度\nTIM2 中断\nUSART2 中断]
    end

    MAIN1 -->|调用| APP1
    MAIN1 -->|调用| APP2
    MAIN1 -->|调用| APP3

    APP1 -->|调用| DRV1
    APP1 -->|调用| DRV2
    APP1 -->|调用| DRV3
    APP1 -->|调用| DRV5
    APP1 -->|调用| DRV6

    APP2 -->|调用| DRV5
    APP2 -->|调用| DRV7
    APP2 -->|调用| DRV6
    APP2 -->|调用| DRV10
    APP2 -->|调用| DRV4

    APP3 -->|调用| DRV10
    APP3 -->|调用| DRV11

    MAIN1 -->|调用| DRV4
    MAIN1 -->|调用| DRV9
    MAIN1 -->|调用| DRV11

    DRV1 -->|软件I2C| DRV8
    DRV2 -->|软件I2C| DRV8
    DRV5 -->|软件I2C| DRV8

    DRV8 -->|GPIO| HW1
    DRV8 -->|GPIO| HW2
    DRV8 -->|GPIO| HW5
    DRV1 -->|I2C PB4/PB5| HW1
    DRV2 -->|I2C PC14/PC15| HW2
    DRV3 -->|USART3 9600| HW3
    DRV4 -->|USART1 9600| HW4
    DRV6 -->|PC13| HW6
    DRV7 -->|PB12~15 PA8| HW7
    DRV7 -->|GPIO 读| HW8
    DRV11 -->|USART2| HW9
```

---

## 2. 模块依赖关系图

```mermaid
graph LR
    main(["main.c\n主控"])

    main --> app_sensor["app_sensor\n传感器业务"]
    main --> app_ui["app_ui\n界面/按键"]
    main --> app_utils["app_utils\n系统工具"]
    main --> gsm["gsm\nGSM短信/LBS"]
    main --> timer["timer\nTIM2/TIM3"]
    main --> usart1["usart1\nGSM串口"]
    main --> usart2["usart2\n语音串口"]
    main --> usart3["usart3\nGPS串口"]

    app_sensor --> VL53L1x["VL53L1x_STM32\n激光测距"]
    app_sensor --> adxl345["adxl345\n加速度计"]
    app_sensor --> GPS["GPS\nRMC解析"]
    app_sensor --> OLED["OLED_I2C\n显示"]
    app_sensor --> wt588d["wt588d\n蜂鸣器"]
    app_sensor --> usart2

    app_ui --> OLED
    app_ui --> wt588d
    app_ui --> gpio["gpio\nGPIO"]
    app_ui --> stmflash["stmflash\nFlash存储"]
    app_ui --> gsm

    app_utils --> stmflash
    app_utils --> usart1

    VL53L1x --> iic["iic\n软件I2C"]
    adxl345 --> iic
    OLED --> iic

    gsm --> usart1
    GPS --> usart3
```

---

## 3. 数据与信号流图

```mermaid
flowchart LR
    subgraph 感知层
        S1([VL53L1X\n激光测距\n0~4500mm])
        S2([ADXL345\n三轴加速度\n±16g])
        S3([GPS\nRMC 报文\n经纬度/时间])
        S4([SIM800C\nLBS 基站坐标\n回退定位])
        S5([光敏传感器\nGM])
        S6([水位传感器\nWATER])
        S7([按键 ×5\nKEY1~KEY5])
    end

    subgraph 处理层
        P1["Get_Distance\n距离读取+限幅\n错误恢复/重初始化"]
        P2["FallDetection\n5次均值采样\n倾斜→2s确认跌倒"]
        P3["Get_GPS\nGPRMC解析\n坐标格式转换"]
        P4["gsm_get_lbs\n基站定位查询\n60s超时"]
        P5["TIM2中断\n光敏同步\n蜂鸣状态机\n水位检测\n刷新节拍"]
        P6["KeySettings\n状态机按键处理\n设置模式切换"]
    end

    subgraph 决策层
        D1{距离 ≤ 安全阈值?}
        D2{持续倾斜 > 2s?}
        D3{GPS/LBS 有效?}
        D4{KEY4 求救?}
        D5{sendSmsFlag != 0?}
    end

    subgraph 输出层
        O1([OLED 显示\n距离/坐标/设置])
        O2([蜂鸣器报警\n多种鸣响模式])
        O3([短信发送\nSIM800C\n含GPS/LBS/未知坐标])
        O4([语音播报\nWT588D/USART2])
        O5([LED 指示\n同步光敏])
        O6([Flash 保存\n阈值+手机号])
    end

    S1 --> P1
    S2 --> P2
    S3 --> P3
    S4 --> P4
    S5 --> P5
    S6 --> P5
    S7 --> P6

    P1 --> D1
    P2 --> D2
    P3 --> D3
    P4 --> D3
    P6 --> D4

    D1 -- 是 --> O1
    D1 -- 是 --> O2
    D1 -- 是 --> O4
    D2 -- 是 --> O2
    D2 -- 是 --> O4
    D2 -- 是 --> D5
    D3 -- GPS锁定 --> O1
    D3 -- LBS有效 --> O1
    D3 -- 无信号 --> O1
    D4 -- 是 --> D5
    P5 --> O5
    P5 --> O2
    D5 -- 是 --> D3
    D3 -->|坐标来源三级回退| O3
    P6 -->|保存设置| O6
    P1 --> O1
```

---

## 4. 中断与主循环协作时序图

```mermaid
sequenceDiagram
    participant HW as 硬件 / 外设
    participant TIM2 as TIM2中断<br/>(每10ms)
    participant USART2IRQ as USART2中断<br/>(语音模块)
    participant MainLoop as 主循环 while(1)
    participant Sensor as app_sensor
    participant UI as app_ui
    participant GSM as gsm

    loop 每 10ms
        TIM2 ->> TIM2: LED=GM 同步光敏
        TIM2 ->> TIM2: BeepUpdate() 蜂鸣状态机
        TIM2 ->> TIM2: 水位检测→StartBeep(5)
        TIM2 ->> MainLoop: timeCount≥2 → refreshFlag=1
        TIM2 ->> TIM2: 200ms → fallTimer--
    end

    loop 每帧 ~20ms (refreshFlag=1)
        MainLoop ->> UI: KeySettings() 按键轮询
        MainLoop ->> UI: ShowHomePage() 主界面刷新
        MainLoop ->> Sensor: Get_GPS() 解析 GPRMC
        MainLoop ->> Sensor: FallDetection() 跌倒判断
        MainLoop ->> Sensor: Get_Distance() 读激光测距
        alt sendSmsFlag != 0
            MainLoop ->> MainLoop: 组装短信内容<br/>(GPS / LBS / 未知)
            MainLoop ->> GSM: sim800_send() 发送短信
        end
    end

    HW -->> USART2IRQ: 语音模块发来 0x31
    USART2IRQ ->> MainLoop: sendSmsFlag = 2（手动求救）

    HW -->> USART2IRQ: 语音模块发来 0x32
    USART2IRQ ->> HW: 回传当前距离(cm)
```

---

## 5. 系统状态机总览

```mermaid
stateDiagram-v2
    [*] --> 初始化

    初始化 --> 正常模式 : 全部外设初始化成功\nalarmEnable=1

    初始化 --> 锁死 : I2C测试失败\n或VL53L1X初始化失败

    正常模式 --> 距离报警 : 测距 ≤ 安全阈值
    距离报警 --> 正常模式 : 测距 > 安全阈值

    正常模式 --> 跌倒报警 : 持续倾斜 > 2s
    跌倒报警 --> 正常模式 : 倾斜消除

    正常模式 --> 紧急求救 : KEY4 按下
    跌倒报警 --> 紧急求救 : 跌倒确认后自动触发

    紧急求救 --> 正常模式 : KEY5 取消

    正常模式 --> 设置模式 : KEY1 进入
    设置模式 --> 阈值设置 : settingMode=1
    阈值设置 --> 手机号设置 : KEY1 继续
    手机号设置 --> 正常模式 : KEY1 保存(Flash写入)

    note right of 紧急求救
        发送求救短信
        坐标三级回退:
        GPS → 基站LBS → 未知
    end note

    note right of 跌倒报警
        蜂鸣器连响8次
        WT588D语音播报
        sendSmsFlag=1
    end note
```

---

## 6. 各模块职责一览

| 模块文件 | 所在层 | 核心职责 |
|---------|--------|---------|
| `main.c` | 主控层 | 系统初始化、主循环调度、TIM2/USART2 中断 |
| `app_sensor.c` | 应用层 | 跌倒检测、激光测距、GPS 数据获取 |
| `app_ui.c` | 应用层 | OLED 界面显示、5 键状态机设置 |
| `app_utils.c` | 应用层 | MCU 首次上电初始化、串口缓冲区清空 |
| `gsm.c` | 驱动层 | SIM800C 短信发送（UTF-8→HEX）、LBS 基站定位 |
| `GPS.c` | 驱动层 | NMEA-0183 GPRMC/GPGGA 解析、UTC→北京时间 |
| `adxl345.c` | 驱动层 | ADXL345 寄存器读写、三轴采样与均值计算 |
| `VL53L1x_STM32.c` | 驱动层 | VL53L1X 初始化、单次测距读取、错误诊断 |
| `OLED_I2C.c` | 驱动层 | SSD1306 字符/汉字显示（软件 I2C） |
| `wt588d.c` | 驱动层 | 蜂鸣器 PWM 状态机（6 种鸣响模式） |
| `iic.c` | 驱动层 | 软件模拟 I2C 时序（Start/Stop/Byte/ACK） |
| `gpio.c` | 驱动层 | GPIO 统一初始化（LED/蜂鸣/按键/传感器） |
| `timer.c` | 驱动层 | TIM2（系统节拍 10ms）、TIM3（超声波计时） |
| `stmflash.c` | 驱动层 | 内部 Flash 读写（手机号 + 安全距离持久化） |
| `usart1/2/3.c` | 驱动层 | 三路串口（GSM / 语音模块 / GPS） |
