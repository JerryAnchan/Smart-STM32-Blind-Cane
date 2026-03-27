# VL53L1X快速参考卡

## 硬件连接 (必须正确!)
```
VL53L1X    →   STM32F103
---------------------------
VDD        →   3.3V
GND        →   GND
SDA        →   PB4
SCL        →   PB5
```

## 代码使用示例

### 1. 包含头文件
```c
#include "VL53L1x_STM32.h"
```

### 2. 初始化 (在main函数中)
```c
// I2C初始化
VL53L1X_I2C_Init();

// 传感器初始化
if(VL53L1X_Init() != 0) {
    // 初始化失败处理
    // 检查硬件连接
}
```

### 3. 读取距离
```c
uint16_t distance_mm;

// 获取距离(单位:毫米)
distance_mm = VL53L1X_GetDistance();

if(distance_mm == 0xFFFF) {
    // 读取失败
} else {
    // 正常距离值
    // distance_mm范围: 0 ~ 4000 mm
}
```

### 4. 完整示例
```c
void main(void) {
    // 系统初始化...
    
    VL53L1X_I2C_Init();
    delay_ms(10);
    
    if(VL53L1X_Init() == 0) {
        while(1) {
            uint16_t dist = VL53L1X_GetDistance();
            
            if(dist != 0xFFFF) {
                // 使用距离值
                printf("Distance: %d mm\n", dist);
            }
            
            delay_ms(100); // 100ms测量间隔
        }
    }
}
```

## 常见问题速查

### Q1: 初始化失败 (返回1)
**可能原因:**
- 检查VDD是否为3.3V
- 检查SDA/SCL是否接对
- 检查上拉电阻 (可能需要4.7kΩ)
- 检查模块是否损坏

### Q2: 读取返回0xFFFF
**可能原因:**
- 被测物体太近 (<30mm)
- 被测物体太远 (>4000mm)
- 被测物体反射率太低
- 环境光太强

### Q3: 距离值不稳定
**可能原因:**
- 被测表面不平整
- 测量角度不垂直
- I2C通信受干扰
- 测量频率过快

### Q4: 与原HC_SR04代码兼容性
当前实现已兼容原代码:
- `currentDistance` 变量单位保持为 mm*10
- 距离阈值等逻辑无需修改
- 显示和报警功能保持不变

## 调试技巧

### 1. 检查I2C通信
```c
uint8_t model_id;
VL53L1X_ReadReg16(0x010F, &model_id);
// 应该读到 0xEA 或 0xEB
```

### 2. 使用测试函数
```c
#include "VL53L1X_Test.c"  // 添加到工程

// 在main函数中调用
VL53L1X_BasicTest();       // 基础测试
VL53L1X_AdvancedTest();    // 高级测试
VL53L1X_I2C_Test();        // I2C测试
```

### 3. OLED显示调试信息
```c
char buf[32];
sprintf(buf, "D:%dmm", VL53L1X_GetDistance());
OLED_ShowStr(0, 0, buf, 2);
```

## 性能参数

| 参数 | 值 |
|------|-----|
| 测量范围 | 0 ~ 4000 mm |
| 测量精度 | ±25 mm (典型) |
| 视场角 | 27° |
| 测量速率 | 最快20ms |
| I2C地址 | 0x52 (8位) |
| 工作电压 | 2.6V ~ 3.5V |
| 工作电流 | 约19mA |

## 注意事项 ⚠️

1. **供电电压**: 必须在2.6V-3.5V范围内，建议3.3V
2. **I2C速度**: 当前实现约100kHz，不要太快
3. **测量频率**: 建议至少间隔100ms
4. **目标反射率**: 目标物体反射率应 >10%
5. **测量角度**: 尽量保持传感器与目标垂直

## 优化建议

### 提高准确度
```c
// 多次测量取平均
uint16_t get_avg_distance(uint8_t samples) {
    uint32_t sum = 0;
    uint8_t valid = 0;
    
    for(uint8_t i = 0; i < samples; i++) {
        uint16_t d = VL53L1X_GetDistance();
        if(d != 0xFFFF) {
            sum += d;
            valid++;
        }
        delay_ms(20);
    }
    
    return valid > 0 ? (sum / valid) : 0xFFFF;
}
```

### 滤波处理
```c
// 简单中值滤波
uint16_t median_filter(uint16_t d1, uint16_t d2, uint16_t d3) {
    if(d1 > d2) { uint16_t t = d1; d1 = d2; d2 = t; }
    if(d2 > d3) { uint16_t t = d2; d2 = d3; d3 = t; }
    if(d1 > d2) { uint16_t t = d1; d1 = d2; d2 = t; }
    return d2;
}
```

---
**版本**: v1.0  
**更新**: 2026-03-27  
**作者**: GitHub Copilot
