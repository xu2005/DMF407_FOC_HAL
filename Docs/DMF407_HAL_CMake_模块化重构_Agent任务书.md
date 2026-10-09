# DMF407 HAL + CMake 模块化重构 · Agent 任务书

## 0. 任务定位

你现在负责继续重构：

```text
DMF407_FOC_HAL
```

目标不是继续围绕一份手写 `.ioc` 打补丁，也不是重新发明一套 FOC。

新的主路线是：

> **以 Template222-100 Legacy 案例的实际行为为基线，重新建立 STM32CubeMX + HAL + CMake 工程；把 MCU 相关代码收口进 BSP，把原 FOC 数学和控制逻辑尽量原样迁移，并在文件夹层面完成模块化。**

---

# 1. 最高原则

## 1.1 Legacy 是行为基线

冻结：

```text
Legacy/Template222-100/
```

原工程只读，不修改。

遇到冲突时证据优先级：

1. 原理图 / IO 分配表
2. Legacy 实际源码
3. Legacy 实际运行行为
4. 注释
5. 分析文档
6. 推测

禁止用“更现代”“更规范”“我觉得合理”覆盖 Legacy 行为而不登记。

## 1.2 不重写 HAL

不要：

- 修改 ST HAL 源码
- 自己重新定义 `HAL_TIM_*`
- 在 Drivers 里塞业务逻辑

正确方式：

```text
STM32 HAL
   ↓
BSP
   ↓
Motor / Control
```

## 1.3 不重写 FOC 数学

以下模块尽可能保持 Legacy：

```text
Clarke
Park
Inverse Park
PI
SVPWM
Current loop
Speed loop
限幅逻辑
方向定义
控制参数
```

修改重点在“底层 API”，不是控制算法。

---

# 2. 当前已经确认的 Legacy 硬件基线

硬件规格以：

```text
DMF407_HAL_引脚时钟配置规范.md
```

为唯一配置入口。

关键配置摘要：

```text
MCU: STM32F407IGT6

HSE 8 MHz
SYSCLK 168 MHz
APB1 Timer = 84 MHz
APB2 Timer = 168 MHz

TIM1:
  PA8  CH1
  PA9  CH2
  PA10 CH3
  PB13 CH1N
  PB14 CH2N
  PB15 CH3N
  PSC=83
  ARR=99
  CenterAligned1
  RCR=0
  Pulse=0
  DeadTime=84
  Lock=1
  Break=Enable
  BreakPolarity=High
  AutomaticOutput=Enable
  TRGO=OC1REF

PF10:
  PM1_CTRL_SD
  GPIO Output
  Legacy initial High
  Pull-down
  50 MHz 对应速度等级

PB12:
  PM1_BRAKE_BKIN
  V1 保持普通 GPIO Input
  NOPULL
  不切 AF1

TIM2:
  PSC=83
  ARR=999
  Up
  IRQ 1/1

TIM3 Encoder:
  PC6 / PC7
  TI12
  PSC=0
  ARR=65535
  Filter=4
  IRQ 1/0

TIM4:
  PSC=10
  ARR=10000
  Up
  IRQ 1/1

ADC1:
  PB1 / IN9 / PM1_VBUS

ADC2:
  PA6 / IN6 / PM1_AMPV

ADC3:
  PA3 / IN3 / PM1_AMPW

ADC1/2/3:
  PCLK2/4
  12 bit
  Scan Disable
  Continuous Enable
  Right
  1 conversion
  TIM1_CC1 Falling
  480 cycles

USART1:
  PB6 TX
  PB7 RX
  115200 8N1
```

特别注意：

```text
PB1 = VBUS
PB0 ≠ VBUS
```

---

# 3. 新工程目标结构

目标结构建议：

```text
DMF407_FOC_HAL/
│
├─ Legacy/
│  └─ Template222-100/
│
├─ Core/
│  ├─ Inc/
│  └─ Src/
│
├─ Drivers/
│
├─ BSP/
│  ├─ bsp_pwm.c
│  ├─ bsp_pwm.h
│  ├─ bsp_adc.c
│  ├─ bsp_adc.h
│  ├─ bsp_encoder.c
│  ├─ bsp_encoder.h
│  ├─ bsp_timer.c
│  ├─ bsp_timer.h
│  ├─ bsp_motor_enable.c
│  ├─ bsp_motor_enable.h
│  ├─ bsp_uart.c
│  └─ bsp_uart.h
│
├─ Control/
│  ├─ clarke.c
│  ├─ clarke.h
│  ├─ park.c
│  ├─ park.h
│  ├─ svpwm.c
│  ├─ svpwm.h
│  ├─ pi.c
│  ├─ pi.h
│  ├─ current_loop.c
│  ├─ current_loop.h
│  ├─ speed_loop.c
│  └─ speed_loop.h
│
├─ Motor/
│  ├─ motor.c
│  ├─ motor.h
│  ├─ motor_foc.c
│  ├─ motor_foc.h
│  └─ motor_config.h
│
├─ Motion/
│  ├─ interpolation.c
│  ├─ interpolation.h
│  ├─ s_curve.c
│  └─ s_curve.h
│
├─ App/
│  ├─ app_main.c
│  ├─ app_main.h
│  ├─ app_state.c
│  └─ app_state.h
│
├─ Docs/
│  ├─ DMF407_HAL_引脚时钟配置规范.md
│  ├─ V1_Legacy_Mapping.md
│  └─ V2_硬件安全修正清单.md
│
├─ CMakeLists.txt
└─ DMF407_FOC_HAL.ioc
```

不要求一次性造完所有空文件。

原则：

> 只有有实际消费者的模块才创建；不要为了“架构漂亮”制造大量空壳。

---

# 4. 模块责任

## 4.1 Core

只保留 CubeMX 生成和必要的 glue code。

不要在 CubeMX 自动生成区塞 FOC 数学。

## 4.2 Drivers

只放 ST HAL / CMSIS。

禁止业务修改。

## 4.3 BSP

BSP 是旧 SPL → HAL 的主要迁移层。

例如旧代码：

```c
TIM_SetCompare1(TIM1, tcm1);
TIM_SetCompare2(TIM1, tcm2);
TIM_SetCompare3(TIM1, tcm3);
```

迁移成：

```c
void BSP_PWM_SetABC(uint16_t u, uint16_t v, uint16_t w)
{
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, u);
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, v);
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_3, w);
}
```

Control / Motor 层不得直接到处操作 TIM1 CCR。

## 4.4 Control

尽量 MCU 无关。

只接收：

```text
电流
角度
速度
参考值
```

输出：

```text
Ud/Uq
PWM duty / compare
```

不要直接包含：

```text
stm32f4xx_hal.h
TIM1
ADC1
GPIO
```

若旧数学代码依赖硬件宏，逐步解耦。

## 4.5 Motor

负责：

```text
Motor 实例
FOC 调度
Current loop
Speed loop
Enable/Disable 逻辑接口
```

V1 不新增复杂状态机。

## 4.6 Motion

Legacy 插补 / S curve 放这里。

V1 允许暂不接到 Motor 主控制路径。

但是 TIM4 硬件基线必须保留。

## 4.7 App

负责系统入口和任务编排。

避免：

```text
main.c 里堆全部业务逻辑
```

---

# 5. CubeMX 重建规则

不要继续以手写 `.ioc` 作为主工作方式。

执行：

1. 打开现有 / 新建干净 CubeMX 工程
2. 选择 STM32F407IGT6
3. 按 `DMF407_HAL_引脚时钟配置规范.md` 配置
4. 使用 GUI 保存
5. Generate Code
6. 检查生成代码
7. 再开始迁移 Legacy

对于 `.ioc` 中不确定键名：

```text
禁止猜
```

尤其：

```text
TIM1 TRGO
MasterSlaveMode
GPIO 枚举
Timer preload
```

全部以 GUI 和生成代码为准。

---

# 6. 迁移顺序

必须按阶段推进。

## Phase 0 · 冻结 Legacy

完成：

- 原案例完整归档
- 不覆盖
- 记录 hash / git commit
- 建 README：Legacy 来源与状态

验收：

```text
Legacy baseline = frozen
```

## Phase 1 · CubeMX + CMake 空工程

只建立：

```text
RCC
SYS
GPIO
USART1
TIM1
TIM2
TIM3
TIM4
ADC1
ADC2
ADC3
```

生成 HAL 工程。

不得移植 FOC。

验收：

```text
CMake 0 error
SWD 可下载
UART 可输出
```

## Phase 2 · BSP

先迁：

```text
PWM
ADC
Encoder
Timer
Motor Enable
UART
```

原则：

```text
一个 BSP 模块迁完 → 编译 → 检查
```

不要一次性复制整个 Legacy。

## Phase 3 · 数学层

迁：

```text
Clarke
Park
InvPark
PI
SVPWM
```

要求：

- 数学代码尽量不改
- 类型变化必须记录
- float 行为保持
- 常量保持
- 限幅保持

## Phase 4 · FOC 调度

接入：

```text
ADC
Encoder
Current loop
Speed loop
SVPWM
PWM compare update
```

保持 Legacy 调度关系。

## Phase 5 · TIM4 / Motion

先保留：

```text
TIM4 IRQ
ARR=10000
PSC=10
```

Motion 可继续 dormant。

等 FOC V1 稳定后再接：

```text
Interpolation
S Curve
XY
```

---

# 7. SPL → HAL 替换表

## PWM

```c
TIM_SetCompare1(TIM1, x)
```

→

```c
__HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, x)
```

## Encoder Count

```c
TIM_GetCounter(TIM3)
```

→

```c
__HAL_TIM_GET_COUNTER(&htim3)
```

## Encoder Direction

```c
TIM3->CR1 & TIM_CR1_DIR
```

→

```c
__HAL_TIM_IS_TIM_COUNTING_DOWN(&htim3)
```

## Timer Start

```c
TIM_Cmd(TIMx, ENABLE)
TIM_ITConfig(TIMx, TIM_IT_Update, ENABLE)
```

→

```c
HAL_TIM_Base_Start_IT(&htimx)
```

## Encoder Start

→

```c
HAL_TIM_Encoder_Start_IT(&htim3, TIM_CHANNEL_ALL)
```

## ADC

V1：

```c
ADC_GetValue(...)
```

→ BSP 内：

```c
HAL_ADC_PollForConversion(...)
HAL_ADC_GetValue(...)
```

不要在 V1 直接改 DMA / Injected。

---

# 8. 中断迁移原则

Legacy ISR：

```text
TIM1
TIM2
TIM3
TIM4
```

V1 先走标准 HAL：

```c
TIMx_IRQHandler()
    ↓
HAL_TIM_IRQHandler()
    ↓
HAL_TIM_PeriodElapsedCallback()
```

业务逻辑不要直接堆在 `stm32f4xx_it.c`。

推荐：

```c
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM1) {
        Motor_FOC_Tick();
    } else if (htim->Instance == TIM2) {
        BSP_Timer_1msTick();
    } else if (htim->Instance == TIM4) {
        Motion_TimerTick();
    }
}
```

TIM3 Encoder 溢出处理按 Legacy 行为迁移。

注意：

最终高频 ISR 是否绕过 HAL 回调属于 V2 性能优化，不在 V1 偷改。

---

# 9. V1 禁止事项

Agent 不得：

- 删除 TIM4
- 把 TIM4 ARR 改成 26715
- 把 PB1 改回 PB0
- 把 PB12 直接改 AF BKIN
- 把 Break Disable
- 把 Pulse 改 50
- 改 PSC/ARR 为“更标准 10 kHz”
- 改 RCR
- 把 ADC 改 Injected
- 把 ADC 改 DMA
- 修改 PI 参数
- 改 SVPWM 数学
- 改 Encoder 极性
- 删除 Legacy 六路 PWM PULLDOWN 行为而不登记
- 在 main.c 一次性重写全部业务
- 把 V2 安全整改混进 V1
- 未实测就宣称电机可以带功率运行

---

# 10. V2 安全整改边界

V1 完成以后，再处理：

```text
BKIN
PF10 安全默认态
R70
Overcurrent
ADC PWM sync
Injected ADC
ISR WCET
RCR
Fault manager
State machine
E-stop
Overvoltage
Undervoltage
Overspeed
Stall
Overtemp
Limit
Control timeout
PM2
TIM8
Dual-axis
```

A0~A4 未闭环前：

```text
A5 = BLOCKED
A6 = BLOCKED
```

---

# 11. 文件级工作纪律

每次修改后必须：

1. 保存
2. 重新从磁盘读回
3. 检查原始行
4. 记录改动文件
5. 编译
6. 报告实际输出

禁止只说：

```text
“已经改好了”
```

必须给：

```text
文件路径
修改内容
关键原始行
构建结果
未完成项
```

如果使用 Git：

```text
git diff
git status
```

必须在报告中总结。

---

# 12. 不允许缓存冒充磁盘证据

之前已出现 `codex-file-preview-*` 旧缓存。

以后验证配置时：

```text
磁盘实际文件 > Preview > 对话附件旧快照
```

若内容冲突：

- 检查绝对路径
- 文件大小
- 修改时间
- SHA256
- Git diff

不要根据 Preview 缓存覆盖磁盘文件。

---

# 13. 构建纪律

每个阶段都要编译。

至少输出：

```text
compiler
preset
build directory
0 error / error
ELF
MAP
BIN
```

F407 使用：

```text
-mcpu=cortex-m4
-mfpu=fpv4-sp-d16
-mfloat-abi=hard
```

不要凭别的工程 preset 名硬套。

先：

```bash
cmake --list-presets
```

再：

```bash
cmake --preset <实际 preset>
cmake --build build/<preset>
```

---

# 14. V1 无功率验收

禁止带功率直流母线。

完成：

- SWD
- UART
- Clock
- GPIO
- PWM 六路
- Complementary
- DeadTime
- TIM1 Update IRQ
- FOC Tick
- TIM2 1 ms
- TIM4 约 1.31 ms
- ADC 原始值
- ADC Poll WCET
- Encoder Count / Direction

任何数值：

```text
实测多少写多少
```

不要为了符合注释修改参数。

---

# 15. 完成定义

V1 只有在以下全部满足后才算完成：

```text
[ ] Legacy frozen
[ ] CubeMX GUI 配置闭环
[ ] HAL code generated
[ ] CMake build 0 error
[ ] BSP migrated
[ ] Math layer migrated
[ ] FOC chain migrated
[ ] No-power PWM validated
[ ] ADC validated
[ ] Encoder validated
[ ] TIM1/2/3/4 IRQ validated
[ ] V1 discrepancy table complete
[ ] V2 items remain separated
```

最终状态应写：

```text
V1 LEGACY-EQUIVALENT HAL BASELINE = PASS
POWER-STAGE ENABLE = BLOCKED
```

直到 V2 安全门禁完成。

---

# 16. Agent 首轮执行任务

本轮只做：

1. 冻结 Legacy
2. 建立 / 整理目录
3. 用 CubeMX GUI 生成 HAL + CMake 最小工程
4. 检查所有生成的初始化函数
5. 建 BSP 骨架
6. 迁移 PWM / ADC / Encoder / Timer / Motor Enable
7. 编译
8. 输出差异报告

本轮不要：

```text
跑电机
上功率母线
改 FOC 参数
做 V2
做双轴
```

---

# 17. 首轮交付物

必须提交：

```text
DMF407_FOC_HAL.ioc
Core/
Drivers/
BSP/
CMakeLists.txt
CMakePresets.json
构建日志
V1_Legacy_Mapping.md
V1_首轮迁移报告.md
```

报告必须写：

```text
完成项
文件级证据
编译结果
Legacy → HAL API 映射
未决项
风险项
下一阶段
```

---

# 18. 最重要的一句话

> **先把 Template222-100 在 HAL+CMake 下“原样活过来”，再谈把它变成更好的电机控制系统。**
