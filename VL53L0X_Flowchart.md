# VL53L0X 程序流程图

本文档描述了在智能盲杖系统中集成 VL53L0X 飞行时间（ToF）激光测距传感器的程序流程。

---

## 一、VL53L0X 初始化流程

```mermaid
flowchart TD
    A([开始初始化]) --> B[拉低 XSHUT 引脚\n使传感器进入硬件复位]
    B --> C[延时 ≥ 10ms]
    C --> D[拉高 XSHUT 引脚\n传感器上电启动]
    D --> E[延时 ≥ 1.2ms\n等待启动完成]
    E --> F[I2C 发送默认地址 0x52\n检测设备是否响应]
    F --> G{ACK 是否收到?}
    G -- 否 --> H[错误处理:\nOLED 显示 VL53L0X Error\n返回失败]
    G -- 是 --> I[读取 MODEL_ID 寄存器\n验证器件型号 0xEE]
    I --> J{型号匹配?}
    J -- 否 --> H
    J -- 是 --> K[执行数据初始化序列\n写入传感器默认配置寄存器]
    K --> L[执行 SPAD 管理\n加载 SPAD 参数]
    L --> M[加载调准配置\n写入相位校准值]
    M --> N[设置测距模式\n高精度模式 / 高速模式 / 长距离模式]
    N --> O[设置时序预算\n例: 33ms / 200ms]
    O --> P[执行参考校准\nVHV 校准 + 相位校准]
    P --> Q([初始化完成])
```

---

## 二、单次测距流程

```mermaid
flowchart TD
    A([触发单次测距]) --> B[向寄存器 0x80 写入 0x01\n启动单次测距]
    B --> C[轮询状态寄存器 0x13\n等待测量结果就绪]
    C --> D{结果就绪?\nbit0 == 1}
    D -- 否 --> E{超时?\n> 500ms}
    E -- 否 --> C
    E -- 是 --> F[返回错误值 0xFFFF]
    D -- 是 --> G[读取距离寄存器\n0x1E 高字节 + 0x1F 低字节]
    G --> H[读取 Range Status\n检查测量有效性]
    H --> I{Range Status == 0?\n测量有效}
    I -- 否 --> J[数据无效\n标记为超量程或遮挡\n返回 0xFFFF]
    I -- 是 --> K[计算距离值 mm\ndistance = reg_val × 1]
    K --> L[清除中断\n写 0x01 至寄存器 0x0B]
    L --> M([返回距离值 mm])
```

---

## 三、主程序集成流程

```mermaid
flowchart TD
    A([系统上电]) --> B[系统基础初始化\ndelay / NVIC / I2C / GPIO]
    B --> C[检查并加载 Flash 配置\n安全距离 / 手机号]
    C --> D[外设初始化\nBEEP / KEY / OLED / ADXL345\nWT588D / GPS / GSM / USART]
    D --> E[VL53L0X 初始化]
    E --> F{初始化成功?}
    F -- 否 --> G[OLED 显示: VL53L0X Err\n蜂鸣器报警\n重试或停机]
    F -- 是 --> H[定时器初始化\nTIM2 10ms 中断]
    H --> I([进入主循环])

    I --> J[按键处理 KeySettings]
    J --> K[显示主界面 ShowHomePage]
    K --> L{settingMode == 0\n且 refreshFlag == 1?}
    L -- 否 --> J
    L -- 是 --> M[清除 refreshFlag]

    M --> N[调用 Get_VL53L0X_Distance]
    N --> O[VL53L0X 单次测距]
    O --> P{距离有效?}
    P -- 否 --> Q[OLED 显示: ToF Error]
    Q --> R[更新跌倒检测 FallDetection]
    P -- 是 --> S[限制最大值 ≤ 4500mm\n更新 currentDistance]
    S --> T[OLED 显示距离值]
    T --> U{emergencyMode == 0?}
    U -- 否 --> V[OLED 显示: 紧急求救]
    V --> R
    U -- 是 --> W{currentDistance / 10\n≤ safetyDistance?}
    W -- 否 --> X[清除 distanceWarning]
    X --> R
    W -- 是 --> Y{distanceWarning == 0?}
    Y -- 否 --> R
    Y -- 是 --> Z[置 distanceWarning = 1\nOLED 显示: 注意障碍\n启动蜂鸣器 StartBeep]
    Z --> R

    R --> AA[更新 GPS 数据 Get_GPS]
    AA --> AB{sendSmsFlag != 0?}
    AB -- 否 --> J
    AB -- 是 --> AC[组装短信内容\n含经纬度信息]
    AC --> AD[调用 sim800_send 发送短信]
    AD --> AE[清除 sendSmsFlag]
    AE --> J
```

---

## 四、TIM2 中断中的定时调度

```mermaid
flowchart TD
    A([TIM2 中断 每 10ms]) --> B[清除中断标志]
    B --> C[更新 LED 状态]
    C --> D[调用 BeepUpdate\n蜂鸣器状态机]
    D --> E{WATER == 1?\n水浸检测}
    E -- 是 --> F[StartBeep 水浸报警]
    E -- 否 --> G{timeCount ≥ 10?\n100ms 周期}
    F --> G
    G -- 否 --> H[timeCount++]
    G -- 是 --> I[timeCount = 0\nrefreshFlag = 1\n通知主循环刷新传感器]
    I --> J{time_count1s ≥ 20?\n200ms 周期}
    H --> J
    J -- 否 --> K[time_count1s++]
    J -- 是 --> L[time_count1s = 0\n倾斜检测时递减 fallTimer\n递减 secondCounter]
    L --> M([中断返回])
    K --> M
```

---

## 五、VL53L0X 与 HC-SR04 对比

| 特性 | HC-SR04（当前方案） | VL53L0X（ToF 方案） |
|------|------------------|-------------------|
| 测距原理 | 超声波时差 | 激光飞行时间 |
| 测距范围 | 2 cm – 450 cm | 3 cm – 200 cm（标准） |
| 精度 | ±3 mm | ±3% |
| 接口 | GPIO 触发 / 回响 | I2C（已有 PC14/PC15） |
| 受干扰因素 | 温度、风速、声噪 | 强光、镜面反射 |
| 功耗 | 较高 | 低（3.3V, < 20mA） |
| STM32 引脚占用 | PB4(Trig) + PB5(Echo) | PC14(SCL) + PC15(SDA) + 1×GPIO(XSHUT) |

> **说明**：VL53L0X 通过 I2C 与 STM32 通信，复用现有 `iic.c` 中的软件 I2C 即可驱动，无需额外修改硬件接口。XSHUT 引脚可连接至任意空闲 GPIO，用于硬件复位传感器。

---

## 六、关键寄存器说明

| 寄存器地址 | 名称 | 作用 |
|----------|------|------|
| 0xC0 | MODEL_ID | 器件型号（应读取 0xEE） |
| 0x80 | SYSRANGE_START | 写 0x01 触发单次测距 |
| 0x13 | RESULT_INTERRUPT_STATUS | bit0=1 表示结果就绪 |
| 0x1E–0x1F | RESULT_RANGE_STATUS(10-11) | 距离结果高低字节（mm） |
| 0x0B | SYSTEM_INTERRUPT_CLEAR | 写 0x01 清除中断 |
| 0x14 | RESULT_RANGE_STATUS | Range Status（0=有效） |

---

> **注意**：本流程图基于 VL53L0X API 标准操作流程设计，结合本项目 STM32F103 平台的软件 I2C 实现。实际移植时需根据 ST 官方 API 文档（UM2029）进行寄存器序列的精确对齐。
