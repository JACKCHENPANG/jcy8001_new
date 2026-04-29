# JCY8001 固件重构记录

> 日期：2026-04-29
> 作者：hmike

---

## 📋 项目背景

JCY8001 阻抗分析仪原固件代码混乱，存在以下问题：
- main.c 为空，业务逻辑分散
- Modbus.c 有数组越界和死循环风险
- 代码结构不清晰，难以维护

**目标**：重构固件代码，从最小通讯系统开始，逐步移植验证。

---

## ✅ 已完成工作

### 1. 编译环境搭建

| 组件 | 版本/路径 |
|------|----------|
| ARM GCC | `/Users/jcy/toolchains/bin/arm-none-eabi-gcc` (13.2.1) |
| J-Link | V9, 序列号 63528769 |
| 目标芯片 | STM32F103RC (256KB Flash, 48KB RAM) |
| 标准外设库 | STM32F10x_StdPeriph_Lib V3.6.0 |
| FreeRTOS | V11.1.0 |

### 2. 项目结构

```
JCY8001_new/
├── Core/
│   ├── main.c
│   ├── stm32f10x.h
│   ├── system_stm32f10x.c
│   └── system_stm32f10x.h
├── Drivers/
│   ├── STM32F10x_StdPeriph_Driver/  # 标准外设库
│   ├── CMSIS/                        # CMSIS 核心和设备文件
│   └── FreeRTOS/                     # FreeRTOS 源码
├── Modules/
│   └── Modbus/
│       ├── modbus.c                  # 简化版 Modbus
│       └── modbus.h
├── Drivers/CMSIS/Device/ST/STM32F1xx/Source/Templates/gcc/
│   └── startup_stm32f103xe.s         # GNU 汇编启动文件
├── linker/
│   └── STM32F103RC.ld                # 链接脚本
├── Makefile
└── NOTES.md                          # 本文件
```

### 3. 关键问题解决

#### 问题 1：HardFault 死循环

**现象**：程序启动后立即进入 HardFault，PC 指向 `ADC1_2_IRQHandler`（死循环）。

**根因**：STM32F103 复位后，部分中断（如 ADC）可能处于挂起状态。启动文件中所有中断向量默认指向 `Default_Handler`（死循环），导致程序卡死。

**解决方案**：在 `SystemInit` 中禁用全局中断并清除挂起中断：

```c
void SystemInit(void) {
    __disable_irq();           // 禁用全局中断
    
    // 清除所有挂起中断
    for (int i = 0; i < 8; i++) {
        NVIC->ICER[i] = 0xFFFFFFFF;  // 清除使能
        NVIC->ICPR[i] = 0xFFFFFFFF;  // 清除挂起
    }
    
    // 后续时钟初始化...
}
```

#### 问题 2：GNU 汇编启动文件

原工程使用 Keil ARM 汇编格式（`.s` 文件），Mac 上使用 ARM GCC 需要 GNU 汇编格式。

**解决方案**：从 STM32CubeF1 复制 GNU 格式启动文件 `startup_stm32f103xe.s`。

#### 问题 3：FreeRTOS 中断映射

FreeRTOS 需要 `PendSV_Handler`、`SVC_Handler`、`SysTick_Handler` 三个中断。

**解决方案**：在 `FreeRTOSConfig.h` 中定义映射：

```c
#define vPortSVCHandler     SVC_Handler
#define xPortPendSVHandler  PendSV_Handler
#define xPortSysTickHandler SysTick_Handler
```

---

## 📊 当前状态

|| 项目 | 状态 |
||------|------|
|| 编译环境 | ✅ 完成 |
|| 最小系统运行 | ✅ 完成 |
|| 串口初始化(USART2) | ✅ 完成 |
|| **Modbus 通讯** | ✅ **验证通过 (2026-04-29)** |
|| FreeRTOS 任务 | ⏳ 待添加 |
|| **DNB1101 阻抗数据读取** | ⏳ 待开始 |

### v2 固件验证 (2026-04-29)

✅ **JCY8001 v2 零依赖固件验证通过**
- 路径: `JCY8001_new/build/firmware.hex`
- 架构: 纯寄存器操作，无 StdPeriph 依赖
- 大小: 1.9KB text
- USART: USART2 (PA2/PA3) @ 115200
- SysTick 修复: 地址偏移已纠正

**Modbus 测试结果:**
```
TX: 01 04 3E 01 00 01 6D E2  (读通道数)
RX: 01 04 02 00 01 78 F0      (正确响应 channel=1)
```

---

## 🔧 编译和烧录命令

```bash
# 编译
cd /Users/jcy/Projects/JCY8001_new
make clean && make

# 烧录
JLinkExe -device STM32F103RC -if SWD -speed 4000 -autoconnect 1 << EOF
loadbin build/firmware.bin 0x08000000
r
g
exit
EOF

# 调试（查看程序状态）
JLinkExe -device STM32F103RC -if SWD -speed 4000 -autoconnect 1 << EOF
h
regs
mem32 0xE000ED28 1
mem32 0xE000ED2C 1
exit
EOF
```

---

## 📋 下一步计划

1. **读取 DNB1101 阻抗数据**
   - 扩展 v2 固件支持 0x3100-0x317F (ZREAL/ZIMAG)
   - 测试读取电压(0x3340)、温度(0x3300)
   - 验证阻抗数据是否正确

2. **验证串口通讯**
   - 恢复 USART1 初始化代码
   - 测试串口发送/接收

3. **FreeRTOS 任务框架**
   - 创建 Modbus 任务
   - 创建测量任务
   - 任务间通信

4. **测量功能移植**
   - DAC 控制
   - ADC 采集
   - 阻抗计算

---

## 📝 经验总结

1. **STM32 启动问题**：复位后中断状态不确定，需要在 SystemInit 中清理
2. **GNU vs Keil 汇编**：语法差异大，需要使用对应的启动文件
3. **最小系统优先**：先让最简单的代码跑起来，再逐步添加功能
4. **调试方法**：J-Link 可以直接查看寄存器和内存，是排查 HardFault 的利器

---

## 🔗 相关文件

- Modbus 协议文档：`~/.hermes/profiles/hmike/skills/embedded/jcy8001-modbus/SKILL.md`
- 原工程路径：`/Users/jcy/Projects/JCY8001/`
- 新工程路径：`/Users/jcy/Projects/JCY8001_new/`

---

## DNB1101 阻抗数据寄存器

### 寄存器地址映射（Modbus 3x 区域 - 功能码04）

| 地址 | 参数 | 类型 | 说明 |
|------|------|------|------|
| 0x3000-0x307F | RE实部 | float | 64通道，每通道4地址(大端float) |
| 0x3080-0x30FF | IM虚部 | float | 64通道，每通道4地址(大端float) |
| 0x3100 | ZREAL Ch0 | int16 | 阻抗实部 × 100 mΩ |
| 0x3140 | ZIMAG Ch0 | int16 | 阻抗虚部 × 100 mΩ |
| 0x3180 | ZVolt Ch0 | int16 | 阻抗电压 × 100 mV |
| 0x3200-0x3201 | VZM Ch0 | int32 | 阻抗模 × 1000 mΩ |
| 0x3280-0x3281 | FREQ Ch0 | int32 | 频率 × 1000 Hz |
| 0x3300 | TEMP Ch0 | int16 | 温度 × 10 °C |
| 0x3340 | VOLT Ch0 | int16 | 电压 × 10000 V |
| 0x3380 | STATUS Ch0 | int16 | 状态标志 |

### 当前 v2 固件支持的寄存器

| 地址 | 参数 | 状态 |
|------|------|------|
| 0x3E00 | 通道数 | ✅ |
| 0x3E01 | 版本号 | ✅ |
| 0x4000 | ZM频率 | ✅ |
| 0x3340 | 电压 | ❌ 待添加 |
| 0x3300 | 温度 | ❌ 待添加 |
| 0x3100 | ZREAL | ❌ 待添加 |
| 0x3140 | ZIMAG | ❌ 待添加 |

### 读取示例

```python
# 读电压 (0x3340, int16)
cmd = [0x01, 0x04, 0x33, 0x40, 0x00, 0x01]
# 响应: [01][04][02][volt_hi][volt_lo][crc_lo][crc_hi]
# 电压值 = (volt_hi<<8 | volt_lo) / 10000 V

# 读ZREAL (0x3100, int16)
cmd = [0x01, 0x04, 0x31, 0x00, 0x00, 0x01]
# 响应: [01][04][02][z_hi][z_lo][crc_lo][crc_hi]
# Z = (z_hi<<8 | z_lo) / 100 mΩ

# 读阻抗模 (0x3200, int32, 2个连续寄存器)
cmd = [0x01, 0x04, 0x32, 0x00, 0x00, 0x02]
# 响应: [01][04][04][v0][v1][v2][v3][crc_lo][crc_hi]
# VZM = (v0<<24 | v1<<16 | v2<<8 | v3) / 1000 mΩ
```
