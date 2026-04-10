# 智能导盲杖 · 报警与通信机制流程图

> 目标平台：STM32F103  
> 报警输出：蜂鸣器（PC13）· WT588D 语音模块（USART2）· OLED 显示  
> 通信输出：SIM800C GSM 模块（USART1）短信

---

## 1. 报警触发总览图

三条独立触发路径最终汇聚到同一条"组装短信→发送"链路。

```mermaid
flowchart TD
    subgraph 触发源
        T1([VL53L1X\n激光测距\nGet_Distance])
        T2([ADXL345\n跌倒检测\nFallDetection])
        T3([WT588D 语音模块\nUSART2 中断\n0x31 指令])
        T4([KEY4 按键\n一键求救\nKeySettings])
        T5([水位传感器\nWATER=0\nTIM2中断])
    end

    subgraph 本地报警
        LA1[蜂鸣器\nStartBeep 3\n短促5次\n距离报警]
        LA2[蜂鸣器\nStartBeep 1\n慢速8次\n跌倒报警]
        LA3[蜂鸣器\nStartBeep 2\n紧急4次\n手动求救]
        LA4[蜂鸣器\nStartBeep 5\n2次短促\n水位报警]
        LA5[OLED\n显示报警类型]
        LA6[语音播报\nSerial_SendByte\n0x33/0x34/0x35]
    end

    subgraph 远程通信
        RC1{sendSmsFlag\n标志位判断}
        RC2[组装短信\n坐标三级回退]
        RC3[sim800_send\nGSM短信发送]
    end

    T1 -- 距离≤阈值 --> LA1
    T1 -- 距离≤阈值 --> LA5
    T1 -- 距离≤阈值 --> LA6

    T2 -- 持续倾斜>2s --> LA2
    T2 -- 持续倾斜>2s --> LA5
    T2 -- 持续倾斜>2s --> LA6
    T2 -- 跌倒确认\nsendSmsFlag=1 --> RC1

    T3 -- 0x31指令\nsendSmsFlag=2 --> RC1
    T4 -- KEY4按下\nsendSmsFlag=2 --> LA3
    T4 -- KEY4按下 --> RC1

    T5 -- WATER=0 --> LA4

    RC1 -- !=0 --> RC2
    RC2 --> RC3
```

---

## 2. 蜂鸣器报警状态机（wt588d.c · BeepUpdate）

蜂鸣器通过 TIM2 中断（每 10ms）驱动，6 种鸣响模式对应不同告警场景。

```mermaid
flowchart TD
    A([BeepUpdate\n每10ms调用]) --> B{beepState\n== IDLE}
    B -- 是 --> Z([直接返回])
    B -- 否 --> C{beepTimer > 0}
    C -- 是 --> D[beepTimer--\n等待计时]
    D --> Z
    C -- 否 --> E{beepState\n== RINGING}
    E -- 是 --> F[BEEP_OUT = 0\n关闭蜂鸣\nbeepState = SILENT\nbeepTimer = beepOffTime]
    E -- 否 --> G[beepCount++]
    G --> H{beepCount\n≥ beepMaxCount}
    H -- 是 --> I[beepState = IDLE\n鸣响结束]
    H -- 否 --> J[BEEP_OUT = 1\n开启蜂鸣\nbeepState = RINGING\nbeepTimer = beepOnTime]
    F --> Z
    I --> Z
    J --> Z

    subgraph 模式参数表
        M0["mode 0: 静音 (直接返回)"]
        M1["mode 1: 慢速 ×8 (150ms开/150ms停) → 跌倒报警"]
        M2["mode 2: 紧急 ×4 (500ms开/50ms停)  → 手动求救"]
        M3["mode 3: 快速 ×5 (1ms开/1ms停)     → 入耳警示"]
        M4["mode 4: 超快 ×8 (15ms开/15ms停)   → 距离报警"]
        M5["mode 5: 双响 ×2 (1ms开/1ms停)     → 水位报警"]
    end
```

---

## 3. 语音模块双向通信协议（USART2 中断）

WT588D / ASR Pro 语音模块与 MCU 之间通过 USART2 互传单字节指令。

```mermaid
sequenceDiagram
    participant MCU as STM32 MCU
    participant WT as WT588D / ASR Pro\n(语音模块)

    Note over MCU,WT: MCU → 语音模块（单向播报指令）
    MCU ->> WT: 0x33  距离警报音
    MCU ->> WT: 0x34  "检测到跌倒"播报
    MCU ->> WT: 0x35  "正在发送求救信号，请耐心等待"

    Note over MCU,WT: 语音模块 → MCU（USART2_IRQHandler）
    WT ->> MCU: 0x31  触发求救短信\n(sendSmsFlag = 2)
    MCU -->> WT: (无回包)

    WT ->> MCU: 0x32  查询当前距离
    MCU -->> WT: ASCII 字符串\n"当前距离cm值"\nUsart2_SendString
```

```mermaid
flowchart TD
    A([USART2 中断触发]) --> B{收到字节\ncom_data}
    B --> C{com_data == 0x31}
    C -- 是 --> D[sendSmsFlag = 2\n标记手动求救短信]
    C -- 否 --> E{com_data == 0x32}
    E -- 是 --> F[sprintf 当前距离 cm\nUsart2_SendString 回传]
    E -- 否 --> G([忽略/其他指令])
    D --> H([退出中断])
    F --> H
    G --> H
```

---

## 4. 定位坐标三级回退机制

```mermaid
flowchart TD
    A([sendSmsFlag != 0\n需要发送短信]) --> B{gpsInitFlag == 1\nGPS 已锁定}
    B -- 是 --> C[使用 GPS 坐标\nGPS.longitude_Degree\nGPS.latitude_Degree]
    B -- 否 --> D{lbs_valid == 1\n基站坐标有效}
    D -- 是 --> E[使用基站 LBS 坐标\nlbs_longitude\nlbs_latitude\n附加标注"(基站)"]
    D -- 否 --> F[坐标填写"未知"]

    C --> G[组装短信:\n跌倒/求救 + 经纬度]
    E --> G
    F --> G
    G --> H[sim800_send\n发送短信]
    H --> I[sendSmsFlag = 0\n清除标志]

    subgraph LBS 周期重试
        J{gpsInitFlag==0\n且 lbs_valid==0} -- 约30秒 --> K[gsm_get_lbs\n重新查询基站坐标]
        K -- 成功 --> L[lbs_valid = 1]
        K -- 失败 --> J
    end
```

---

## 5. GSM 模块初始化与基站定位流程（gsm.c）

```mermaid
flowchart TD
    subgraph gsm_init["gsm_init · 短信功能初始化"]
        A1([开始]) --> A2[uart1_send\nconfig,set,smson,1,0,0,0,0,1,1]
        A2 --> A3{gsm_rev_okflag==1\n收到 OK 回包}
        A3 -- 否 --> A4{waittry > 3000\n3秒超时}
        A4 -- 否 --> A3
        A4 -- 是 --> A2
        A3 -- 是 --> A5([初始化完成])
    end

    subgraph gsm_lbs_init["gsm_lbs_init · 基站定位初始化"]
        B1([开始]) --> B2[uart1_send\nconfig,set,location,1,1,60,0,0,0,0]
        B2 --> B3{gsm_rev_okflag==1\n5秒超时等待}
        B3 -- 超时/成功 --> B4([返回])
    end

    subgraph gsm_get_lbs["gsm_get_lbs · 查询基站坐标"]
        C1([开始]) --> C2[清空接收缓冲区\nuart1_send: config,get,lbsloc]
        C2 --> C3{gsm_rev_okflag==1\n最长60秒等待}
        C3 -- 超时 --> C4([返回 0\n失败])
        C3 -- 收到OK --> C5[delay_ms 300\n等待剩余字节]
        C5 --> C6[查找 lbsloc,ok,]
        C6 --> C7{找到标记}
        C7 -- 否 --> C4
        C7 -- 是 --> C8[atof 解析经度\natof 解析纬度]
        C8 --> C9{lon==0 且 lat==0}
        C9 -- 是 --> C4
        C9 -- 否 --> C10([返回 1\n成功，经纬度已写入])
    end
```

---

## 6. 短信发送完整链路（gsm.c · sim800_send + gsm_send_msg）

```mermaid
flowchart TD
    A([sim800_send\n传入 UTF-8 内容]) --> B[OLED 第6行\n显示 Send SMS...]
    B --> C["utf8_to_hexstr\nUTF-8 字节逐字节转\n十六进制 ASCII 串\n(例 E5 88 86 → E588862...) "]
    C --> D["snprintf 拼接指令\nconfig,set,sms,手机号,HEX内容"]
    D --> E[uart1_send\n通过 USART1 发出]
    E --> F[delay_ms 500\n初始等待]
    F --> G{gsm_rev_okflag==1\n收到模块 OK 回包}
    G -- 否 --> H{send_count > 8000\n8秒超时}
    H -- 否 --> G
    H -- 是 --> I[send_error = 1]
    I --> J[OLED 显示\nSend FAIL! Retry]
    G -- 是 --> K[send_error = 0]
    J --> L{retry < 3\n最多重试3次}
    K --> L
    L -- 是 --> M[retry++\n重置计数器]
    M --> B
    L -- 否 --> N{send_error == 1}
    N -- 是 --> O[OLED 显示\nSend FAIL!]
    N -- 否 --> P[OLED 显示\nSend OK!]
    O --> Q[delay_ms 500\n清空状态行]
    P --> Q
    Q --> Z([返回])
```

---

## 7. 报警与通信端到端时序图

展示从事件发生到短信送达的完整时间线（以跌倒场景为例）。

```mermaid
sequenceDiagram
    participant ADXL as ADXL345\n加速度计
    participant TIM2 as TIM2中断\n(每10ms)
    participant APP as FallDetection\n(每20ms)
    participant BEEP as 蜂鸣器
    participant WT as WT588D\n语音模块
    participant OLED as OLED 显示
    participant GSM as SIM800C\nGSM模块
    participant Phone as 紧急联系人\n手机

    Note over ADXL,APP: 阶段一：倾斜检测（0ms 起）
    loop 每20ms
        APP ->> ADXL: adxl345_read_average (5次采样)
        ADXL -->> APP: ax, ay, az 均值
        APP ->> APP: |ax|≥190 或 |ay|≥190 → tiltDetected=1
    end

    Note over TIM2,APP: 阶段二：跌倒计时（200ms精度）
    loop 每200ms（TIM2内20次计数）
        TIM2 ->> TIM2: tiltDetected=1 → fallTimer--
    end

    Note over APP,Phone: 阶段三：跌倒确认（fallTimer归零，约2s后）
    APP ->> APP: fallTimer==0 → fallDetected=1
    APP ->> OLED: 显示"跌倒"字样
    APP ->> WT: Serial_SendByte(0x34)\n"检测到跌倒"语音播报
    APP ->> BEEP: StartBeep(1)\n慢速蜂鸣×8 (150ms/150ms)
    APP ->> APP: sendSmsFlag = 1

    Note over APP,Phone: 阶段四：短信组装与发送（主循环 refreshFlag 触发）
    APP ->> APP: 检测 sendSmsFlag != 0
    APP ->> WT: Serial_SendByte(0x35)\n"正在发送求救信号"
    APP ->> APP: 查询 gpsInitFlag / lbs_valid\n三级回退确定坐标
    APP ->> GSM: sim800_send\n发送"注意,检测到用户跌倒,经度:xxx,纬度:xxx"
    GSM -->> APP: OK 回包 (≤8秒)
    GSM ->> Phone: 📩 短信送达

    Note over APP,OLED: 阶段五：结果反馈
    APP ->> OLED: Send OK! / Send FAIL!
    APP ->> APP: sendSmsFlag = 0
```

---

## 8. 三种告警场景对比表

| 场景 | 触发条件 | 蜂鸣模式 | 语音指令 | 短信内容前缀 | sendSmsFlag |
|------|---------|---------|---------|------------|------------|
| **距离报警** | 测距 ≤ 安全阈值 | mode 3（快速5次） | 0x33 | — (不发短信) | 不变 |
| **跌倒报警** | 持续倾斜 > 2s | mode 1（慢速8次） | 0x34 | "注意,检测到用户跌倒" | = 1 |
| **手动求救** | KEY4 / WT 0x31 | mode 2（紧急4次） | — | "用户主动求救,需要紧急救援" | = 2 |
| **水位报警** | WATER=0（TIM2） | mode 5（双响2次） | — | — (不发短信) | 不变 |
| **取消报警** | KEY5 / 倾斜消除 | StopBeep() | — | — | 不变 |
