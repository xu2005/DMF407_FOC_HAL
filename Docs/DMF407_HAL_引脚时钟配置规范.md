# DMF407 HAL · 引脚 / 时钟 / 外设配置规范
## Template222-100 Legacy 等价迁移基线

> 适用项目：`DMF407_FOC_HAL`
>
> 目标平台：STM32F407IGT6 + ATK-DMF407 + ATK-PD6010B
>
> 迁移目标：将 `Template222-100` 的 Keil + STM32F4 SPL 工程迁移为 **STM32CubeMX + HAL + CMake**，第一阶段只建立 **V1 Legacy 等价基线**。
>
> 本文是硬件初始化规格书，不是安全整改清单。  
> **V1 以 Legacy 实际行为为基线；2026-10-10 明确裁决的 D-20 为安全例外：PF10 默认 Low 已前移到 V1。保护链、ADC 同步采样、启动状态机、故障管理等仍属 V2。**

---

# 1. 总体原则

## 1.1 证据优先级

发生冲突时按以下顺序裁决：

1. 原理图 / IO 分配表
2. Template222-100 / Legacy 源码的实际寄存器配置
3. Legacy 实际运行行为
4. Legacy 注释
5. 项目分析文档
6. 推测

**旧代码是 V1 默认模板。没有充分证据，不改变控制行为。**

## 1.2 V1 / V2 分界

### V1 允许

- SPL → HAL API 替换
- Keil → CMake
- 手写初始化 → CubeMX 生成初始化
- 建立 BSP / Driver 封装
- 目录模块化
- 将寄存器访问收口进 BSP
- 修复“无法在新工程中物理共存”的配置冲突，但必须登记差异
- 无功率条件下做波形、ADC、Encoder、IRQ、构建验证

### V1 不允许偷偷做

- 改 FOC 数学
- 改 PI 参数
- 改 PWM 频率
- 改控制周期
- 改 ADC 架构为 Injected / DMA
- 把 PB12 直接修成有效 BKIN
- 除 D-20 默认 Low / 重复 ForceDisable 外，扩展 PF10 使能和启动安全逻辑
- 改 RCR
- 改故障状态机
- 改双轴资源分配
- 为了“看起来更合理”而清理 Legacy 副作用

这些属于 V2。

---

# 2. MCU 与工程基础

| 项目 | 配置 |
|---|---|
| MCU | STM32F407IGT6 |
| Package | LQFP176 |
| HAL Package | STM32Cube FW_F4 |
| Toolchain | CMake |
| Debug | Serial Wire |
| SWDIO | PA13 |
| SWCLK | PA14 |
| Heap | 0x200 |
| Stack | 0x400 |

建议工程名：

```text
DMF407_FOC_HAL
```

建议保留：

```text
Legacy/
```

目录中的原始案例必须冻结，只读，不覆盖。

---

# 3. RCC / 系统时钟

Legacy 的系统时钟目标保持不变。

| 参数 | V1 配置 |
|---|---:|
| HSE | 8 MHz Crystal |
| PLL Source | HSE |
| PLLM | 8 |
| PLLN | 336 |
| PLLP | 2 |
| PLLQ | 7 |
| SYSCLK | 168 MHz |
| AHB Prescaler | /1 |
| HCLK | 168 MHz |
| APB1 Prescaler | /4 |
| PCLK1 | 42 MHz |
| APB1 Timer Clock | 84 MHz |
| APB2 Prescaler | /2 |
| PCLK2 | 84 MHz |
| APB2 Timer Clock | 168 MHz |
| Flash Latency | 5 WS |
| Voltage Scale | Scale 1 |

注意：

- TIM1 来自 APB2 Timer Clock = **168 MHz**
- TIM2 / TIM3 / TIM4 来自 APB1 Timer Clock = **84 MHz**
- Legacy 注释中把 PCLK2 84 MHz 当成 TIM1 Timer Clock 的写法不要沿用
- 不要为了“凑 10 kHz”擅自改 PSC / ARR

---

# 4. V1 引脚总表

## 4.1 系统 / 调试

| 引脚 | CubeMX 功能 | 用途 |
|---|---|---|
| PH0 | RCC_OSC_IN | HSE 8 MHz |
| PH1 | RCC_OSC_OUT | HSE 8 MHz |
| PA13 | SYS_SWDIO | SWD |
| PA14 | SYS_SWCLK | SWD |

## 4.2 PM1 三相互补 PWM

| 引脚 | 功能 | 板卡网络 |
|---|---|---|
| PA8 | TIM1_CH1 | PM1_PWM_UH |
| PA9 | TIM1_CH2 | PM1_PWM_VH |
| PA10 | TIM1_CH3 | PM1_PWM_WH |
| PB13 | TIM1_CH1N | PM1_PWM_UL |
| PB14 | TIM1_CH2N | PM1_PWM_VL |
| PB15 | TIM1_CH3N | PM1_PWM_WL |

### Legacy GPIO 行为

由于 `time1.c` 复用了同一个 `GPIO_InitStructure`，六路 TIM1 PWM AF 引脚实际继承了：

```text
Pull = PULLDOWN
Speed = 50 MHz
```

V1 以“实际寄存器行为”为准，保留这一行为。

HAL / CubeMX 等价目标：

```text
GPIO Pull   = Pull-down
GPIO Speed  = High / 50 MHz 对应级别
GPIO Mode   = Alternate Function Push-Pull
GPIO AF     = AF1 TIM1
```

是否在 V2 清理为 NOPULL，后续单独决定。

## 4.3 驱动控制 / Brake

| 引脚 | V1 功能 | 网络 |
|---|---|---|
| PF10 | GPIO Output | PM1_CTRL_SD |
| PB12 | GPIO Input | TIM1_BKIN / 刹车网络 |

### PF10

Legacy 初值 High；本轮 D-20 按用户已裁决的安全例外将可下载 HAL 固件改为：

```text
Mode        = Output Push-Pull
Pull        = Pull-down
Speed       = High / 50 MHz 对应级别
Initial     = Low（GPIO_PIN_RESET）
```

注意：

- 这是相对 Legacy 的有意安全偏离：CubeMX `.ioc` 初始 Low，生成代码在 GPIO 输出模式前预置 RESET，main 经 BSP 重复禁能；无置高使能 API
- 依据 PD6010B_V1.0 原理图，板级 CTRL_SD=0 是目标禁止态；实板版本、R70、上电/复位窗口仍需无功率实测，软件 Low 不代表硬件安全通过
- V2 仍需完成自检、状态机授权、故障管理与保护链；R-03 / SF-02 未关闭，POWER-STAGE ENABLE BLOCKED

### PB12

V1 保持：

```text
Mode = GPIO Input
Pull = No Pull
EXTI = Disabled
AF   = 不配置
```

即：

> TIM1 的 BDTR 中 Break Enable，但 PB12 不切到 AF1，所以 BKIN 实际不进入 TIM1。

这是 Legacy 行为复刻，不是正确的最终安全设计。V2-SF-01 决定是否真正接通 BKIN。

---

# 5. TIM1 · 三相互补 PWM

Legacy：

```text
TIM1_Int_Init(100-1, 84-1)
```

## 5.1 时基

| 项目 | V1 |
|---|---:|
| Clock | 168 MHz |
| Prescaler | 83 |
| Counter Period | 99 |
| Counter Mode | Center Aligned Mode 1 |
| Clock Division | DIV1 |
| Repetition Counter | 0 |
| Auto Reload Preload | Enable |

计数节拍：

```text
168 MHz / (83 + 1) = 2 MHz
```

中心对齐完整 PWM 周期约：

```text
0 → ARR → 0
```

ARR=99 时约 10.1 kHz。

**不要把 PWM 基波频率、Update Event 频率、FOC ISR 频率当成同一个数。**

## 5.2 CH1 / CH2 / CH3

三组配置一致：

```text
PWM Mode            = PWM1
Output State        = Enable
Complementary State = Enable
OC Polarity         = High
OCN Polarity        = High
Idle State          = Reset
N Idle State        = Reset
Pulse               = 0
```

Legacy 没有设置 `TIM_Pulse`，因此初始 CCR = 0。

V1 必须保持：

```text
CH1 Pulse = 0
CH2 Pulse = 0
CH3 Pulse = 0
```

## 5.3 BDTR

| 参数 | V1 |
|---|---|
| OSSR | Disable |
| OSSI | Disable |
| Lock Level | Level 1 |
| Dead Time | 84 |
| Break State | Enable |
| Break Polarity | High |
| Automatic Output | Enable |

注意：

```text
DeadTime = 84
```

是 DTG 编码，不等于 84 ns。

## 5.4 Master Output Trigger

Legacy：

```c
TIM_SelectOutputTrigger(TIM1, TIM_TRGOSource_OC1Ref);
```

HAL 生成代码应最终出现：

```c
sMasterConfig.MasterOutputTrigger = TIM_TRGO_OC1REF;
sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
```

### 重要规则

不要依赖手工猜测 `.ioc` 键名。

必须：

1. 在 CubeMX GUI 中打开 TIM1
2. 进入 Master/Slave Configuration
3. 将 Trigger Output (TRGO) 设为 OC1REF
4. Master/Slave Mode 设为 Disable
5. 保存
6. 生成代码后检查 `MX_TIM1_Init()`

GUI / 生成代码是最终证据。

## 5.5 TIM1 IRQ

```text
TIM1_UP_TIM10_IRQn
Preemption Priority = 0
Sub Priority        = 0
```

V1 使用：

```c
HAL_TIM_Base_Start_IT(&htim1);
```

以及标准：

```c
HAL_TIM_IRQHandler()
HAL_TIM_PeriodElapsedCallback()
```

是否改轻量 IRQ 路径属于 V2。

---

# 6. TIM2 · Legacy 1 ms 时基

Legacy：

```text
Time2_Init(999, 83)
```

配置：

| 项目 | V1 |
|---|---:|
| Clock | 84 MHz |
| Prescaler | 83 |
| Period | 999 |
| Counter Mode | Up |
| Clock Division | DIV1 |
| Auto Reload Preload | Disable |
| Channels | Disable |

理论：

```text
84 MHz / 84 = 1 MHz
1000 counts = 1 ms
```

IRQ：

```text
TIM2_IRQn
Preemption = 1
Sub        = 1
```

注意：

- TIM2 在 V1 按 Legacy 保留
- 第二轴 PM2 Encoder 使用 PA15 / PB3 = TIM2_CH1 / CH2
- 进入双轴后，TIM2 必须让给 PM2 Encoder
- V2 / 双轴阶段建议把 1 ms 调度迁到 TIM6 / TIM7

---

# 7. TIM3 · PM1 Encoder

引脚：

```text
PC6 = TIM3_CH1
PC7 = TIM3_CH2
```

配置：

| 项目 | V1 |
|---|---|
| Combined Channels | Encoder Mode |
| Encoder Mode | TI1 and TI2 |
| Prescaler | 0 |
| Period | 65535 |
| Counter Mode | Up |
| IC1 Polarity | Rising |
| IC2 Polarity | Rising |
| IC1 Prescaler | DIV1 |
| IC2 Prescaler | DIV1 |
| IC1 Filter | 4 |
| IC2 Filter | 4 |
| Auto Reload Preload | Disable |

GPIO：

```text
Mode  = AF TIM3
Pull  = NOPULL
Speed = Legacy 对应 50 MHz
```

IRQ：

```text
TIM3_IRQn
Preemption = 1
Sub        = 0
```

V1 HAL：

```c
HAL_TIM_Encoder_Start_IT(&htim3, TIM_CHANNEL_ALL);
```

读数：

```c
__HAL_TIM_GET_COUNTER(&htim3);
```

方向：

```c
__HAL_TIM_IS_TIM_COUNTING_DOWN(&htim3);
```

---

# 8. TIM4 · Legacy 插补时基

Legacy 真实调用顺序：

```text
speed_period_max 初始值 = 10000
Time4_Init(speed_period_max, 10)
cul_Saccdec()
```

因此：

> TIM4 初始化时拿到的 ARR 是 10000。  
> `cul_Saccdec()` 后来算出的其他值没有重新写入 TIM4。

V1：

| 项目 | 配置 |
|---|---:|
| Clock | 84 MHz |
| Prescaler | 10 |
| Period | 10000 |
| Counter Mode | Up |
| Clock Division | DIV1 |
| Repetition Counter | 0 |
| Auto Reload Preload | Disable |
| Channels | Disable |

IRQ：

```text
TIM4_IRQn
Preemption = 1
Sub        = 1
```

理论更新周期约：

```text
84 MHz / (10 + 1) ≈ 7.636 MHz
(10000 + 1) / 7.636 MHz ≈ 1.31 ms
```

V1 保留 TIM4 外设和 IRQ。

Motion / 插补模块可以暂时不接入主动控制路径，但不能在 V1 偷偷删除 Legacy 已存在的外设关系。

---

# 9. ADC1 / ADC2 / ADC3

## 9.1 引脚

| ADC | Channel | Pin | 网络 |
|---|---|---|---|
| ADC1 | IN9 | PB1 | PM1_VBUS |
| ADC2 | IN6 | PA6 | PM1_AMPV |
| ADC3 | IN3 | PA3 | PM1_AMPW |

特别注意：

```text
PB1 = PM1_VBUS
PB0 ≠ VBUS
```

PB0 是 PM1_AMPU。

## 9.2 V1 公共配置

三个 ADC 都按 Legacy 配置：

```text
Clock Prescaler       = PCLK2 / 4
Resolution             = 12 bit
Scan Conversion        = Disable
Continuous Conversion  = Enable
Data Alignment         = Right
Nbr Of Conversion      = 1
External Trigger       = TIM1_CC1
External Trigger Edge  = Falling
Sampling Time          = 480 Cycles
DMA                     = Disabled
```

GPIO：

```text
PA3 / PA6 / PB1
Mode = Analog
Pull = NOPULL
```

### V1 读取行为

保留 Legacy 的阻塞式行为：

```c
HAL_ADC_PollForConversion(...)
HAL_ADC_GetValue(...)
```

不要在 V1 改成 Injected / DMA。

### V2

V2 才改为：

```text
TIM1 指定 PWM 相位
→ ADC Injected Trigger
→ ADC 转换完成
→ 直接读 JDR
→ FOC
→ 更新 PWM
```

---

# 10. USART1 · 调试串口

Legacy 中 PA9 / PA10 的 USART1 初始化代码未实际启用，而且与：

```text
PA9  = TIM1_CH2
PA10 = TIM1_CH3
```

存在物理冲突。

V1 明确登记差异后使用：

```text
PB6 = USART1_TX
PB7 = USART1_RX
```

参数：

```text
Mode         = Asynchronous
Baud         = 115200
Word Length  = 8 bits
Parity       = None
Stop Bits    = 1
HW Flow      = None
Mode         = TX_RX
Oversampling = 16
```

GPIO：

```text
Alternate Function = USART1
Pull               = NOPULL
Speed              = 合理高速等级
```

不要在 FOC 高频 ISR 中阻塞 printf。

---

# 11. NVIC

V1：

| IRQ | Preemption | Sub |
|---|---:|---:|
| TIM1_UP_TIM10 | 0 | 0 |
| TIM3 | 1 | 0 |
| TIM2 | 1 | 1 |
| TIM4 | 1 | 1 |
| SysTick | 15 | 0 |

Priority Group：

```text
NVIC_PRIORITYGROUP_4
```

注意：

GROUP4 下全部优先级位用于抢占优先级，所谓 `(1,0)` 与 `(1,1)` 的子优先级在硬件上并不会形成真正的次级排序。

V1 只记录，不重新设计。

---

# 12. V1 必须保持的控制行为

以下项目在 HAL 重构时不得修改：

```text
PWM PSC / ARR
Center Aligned Mode
RCR
DeadTime
Pulse 初始值
ADC 通道
ADC Continuous 行为
ADC Trigger
ADC Sample Time
Encoder 极性 / Filter
TIM2 周期
TIM4 周期
PI 参数
Clarke / Park / InvPark
SVPWM
限幅
方向约定
Speed loop / Current loop 的原有分频关系
```

API 可换，例如：

```c
TIM_SetCompare1(TIM1, x);
```

→

```c
__HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, x);
```

但控制含义不变。

---

# 13. V2 明确不在本规范实施范围内

以下全部属于 V2：

- PB12 是否真正配置 TIM1_BKIN AF1
- BKIN 极性与恢复流程
- PF10 实板禁止态及完整使能保护链（D-20 软件默认 Low / 重复禁能已前移至 V1）
- PD6010B R70 锁存 / 周期保护确认
- ADC 注入组 / PWM 同步采样
- ISR WCET 优化
- TIM1 RCR 是否改为 1
- SYS_INIT / IDLE / READY / RUNNING / FAULT / ESTOP 状态机
- Overcurrent / Overvoltage / Undervoltage / Encoder / Overspeed / Stall / Overtemp / Limit / Timeout
- 双轴 TIM8 / TIM2 资源重排
- PM2 FOC
- 安全功率测试

在 A0~A4 未闭环前：

```text
A5 单轴功率运行 = BLOCKED
A6 双轴运动     = BLOCKED
```

---

# 14. CubeMX 操作纪律

以后不要把“手写 `.ioc`”当作正常配置方法。

正确流程：

```text
规格书
  ↓
CubeMX GUI
  ↓
保存 .ioc
  ↓
Generate Code
  ↓
检查生成的 HAL 初始化代码
  ↓
CMake 编译
  ↓
无功率实测
```

如果 GUI 与手写 `.ioc` 不一致：

> **以 CubeMX GUI 保存结果 + 生成代码为准。**

尤其必须 GUI 确认：

- TIM1 TRGO = OC1REF
- MasterSlaveMode = Disable
- TIM1 CH1/2/3 complementary PWM
- Break / DeadTime
- TIM2/3/4 ARPE
- ADC2/ADC3 与 ADC1 公共参数完全一致
- GPIO Pull / Speed
- USART1 8N1

---

# 15. 生成后验收

生成后逐项检查：

```text
MX_GPIO_Init()
MX_ADC1_Init()
MX_ADC2_Init()
MX_ADC3_Init()
MX_TIM1_Init()
MX_TIM2_Init()
MX_TIM3_Init()
MX_TIM4_Init()
MX_USART1_UART_Init()
```

必须核：

```text
SystemClock_Config
TIM1 PSC/ARR/CMS/RCR/ARPE
TIM1 CH1/2/3 Pulse=0
TIM1 BDTR
TIM1 TRGO=OC1REF
TIM2 PSC83 ARR999
TIM3 Encoder TI12 / Filter4
TIM4 PSC10 ARR10000
ADC1/2/3 全参数
GPIO Pull/Speed
NVIC
USART1
```

然后：

```bash
cmake --list-presets
cmake --preset <实际存在的 preset>
cmake --build build/<preset>
```

目标：

```text
0 error
ELF / MAP / BIN 正常生成
```

---

# 16. V1 无功率验收

**驱动器功率直流侧不通电。**

至少记录：

| 项目 | 要求 |
|---|---|
| CMake | 0 error |
| SWD | 可下载 / 单步 |
| UART | 115200 正常 |
| PF10 | D-20 目标初始 Low、复位窗口与实板禁止态待无功率验证 |
| PWM | 六路互补波形 |
| DeadTime | 可观察 |
| PWM 基波 | 实测 |
| TIM1 Update IRQ | 实测 |
| FOC 调度频率 | 实测 |
| ADC | 原始值 / 零点 |
| ADC_ProcessData WCET | 实测 |
| Encoder | 方向 / Count |
| TIM2 | 1 ms 调度 |
| TIM4 | 约 1.31 ms IRQ |

**V1 的成功标准不是“电机转起来”，而是“Legacy 行为在 HAL 工程里可复现、可观察、可编译”。**
