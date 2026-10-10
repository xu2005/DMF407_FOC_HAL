# V1 · Legacy 等价核对表 — DMF407_FOC_HAL

> **本表只覆盖 V1 范围：SPL→HAL、Keil→CMake、手写初始化→CubeMX。零控制行为变更。**
>
> 任何"顺手整改"都不属于本表。整改项全部登记在 [`V2_硬件安全修正清单.md`](V2_硬件安全修正清单.md)。

- **目标 MCU**：STM32F407IGT6（LQFP176）
- **Legacy 基线**：A 版 `enable_flag=1` / `Speed_Kp=0.08` / `Speed_Ki=0.0015`
- **Legacy 源码**：`DMF407_FOC_HAL\Legacy\`（已归档）
- **`.ioc`**：`DMF407_FOC_HAL.ioc`，**手写、未经 CubeMX 验证**，以 GUI 实际显示为准

---

## 证据优先级

> **原理图 / IO 分配表 > 旧代码实际寄存器配置 > 旧代码运行行为 > 旧代码注释 > 我们自己的推测**

冲突时按此顺序裁决，并把裁决过程记进第 10 节。

**旧代码就是模板，也是默认答案。没有充分证据，不改变配置。**

---

## V1 的四条纪律

1. **API 可以换，控制行为不要先换。** `TIM_SetCompare1/2/3()` → `__HAL_TIM_SET_COMPARE()` 这类是纯 API 替换。

2. **V1 可以换实现机制，不能擅自删 Legacy 已存在的外设和调度关系。**
   Legacy 里真实存在的外设（TIM1/2/3/4、ADC1/2/3、GPIO、NVIC 优先级）一律复刻。"算法不移植"与"外设基线不复刻"是两回事。
   现实理由：如果模板里某个状态机、延时逻辑或变量隐含依赖某外设，提前删掉等于多引入一个故障变量。

3. **只有在下列三类情况下允许 V1 不照抄 Legacy**，且必须在第 10 节登记 D-xx，不得静默修改：
   - **(a) 物理上不可能照抄** —— 例如 USART1 的 PA9/PA10 与 TIM1 PWM 发生真实引脚冲突；
   - **(b) SPL→HAL/CubeMX 机制变化导致无法逐寄存器照搬** —— 但最终寄存器/行为要尽量等价；
   - **(c) 明确确认是代码笔误且在 CubeMX 里表达不出来**。

4. **V1 的验收是"配置等价 + 能编译 + 无功率可观测"，不是"电机能转"。** 带功率测试属于 V2 之后。

> 其余情况一律：**先照 Legacy 做出来，跑通，然后再改。**

---

## 0. 前置条件

- [x] STM32CubeMX（已装 `D:\STM32CubeMX`，6.12.0-RC9）
- [x] **`STM32Cube_FW_F4_V1.28.3` 固件包已安装且完整** ✓ 已核验

  位置 `%USERPROFILE%\STM32Cube\Repository\STM32Cube_FW_F4_V1.28.3\`；`package.xml` 为 `Release="FW.F4.1.28.0" Patch="FW.F4.1.28.3"`；`Drivers\STM32F4xx_HAL_Driver\Inc` 99 个头文件；`Drivers\CMSIS\Device\ST\STM32F4xx\Source\Templates\gcc\startup_stm32f407xx.s` 存在。
  `.ioc` 已指向 `STM32Cube FW_F4 V1.28.3` 且 `LastFirmware=true`，**打开时不应再提示缺包**。

---

## 1. Project Manager

- [ ] Project Name：`DMF407_FOC_HAL`
- [ ] Project Location：`C:\Users\xuhengyi\Desktop\item\课设\DMF407_FOC_HAL`
- [ ] Toolchain / IDE：`CMake` ★
- [ ] Heap `0x200` / Stack `0x400`
- [ ] Advanced Settings 里 TIM1/TIM2/TIM3 的 **Generate IRQ Handler 保持勾选**

  V1 走标准 `HAL_TIM_IRQHandler` → `HAL_TIM_PeriodElapsedCallback` 路径。ISR 开销优化属于 V2。

---

## 2. 时钟树

| 项 | 值 | Legacy 出处 |
|---|---|---|
| HSE | 8 MHz 晶振 | `USER/stm32f4xx.h:123` `HSE_VALUE=8000000` |
| PLL Source / M / N / P / Q | HSE / 8 / 336 / 2 / 7 | `USER/system_stm32f4xx.c` |
| SYSCLK | 168 MHz | 同上 |
| AHB / APB1 / APB2 | /1 / /4 / /2 | `RCC_CFGR_HPRE_DIV1` / `PPRE1_DIV4` / `PPRE2_DIV2` |
| Flash Latency | 5 WS | `FLASH_ACR_LATENCY_5WS` |
| VOS | Scale 1 | `PWR_CR_VOS` |
| **TIM1 定时器时钟** | **168 MHz**（APB2×2） | 记下来，第 4 节要用 |
| **TIM2/TIM3 定时器时钟** | **84 MHz**（APB1×2） | 记下来，第 5/6 节要用 |

- [ ] ★ HSE / PLL / AHB / APB1 / APB2 / Flash latency / VOS 不出现红色冲突
- [ ] Clock Configuration 右上角 HCLK 显示 **168 MHz**

> 旧工程 `main.c:59` 注释写"84M/100/84=10k"，把 APB2 的 PCLK（84 MHz）当成了定时器时钟。实际是 168 MHz。
> **不要按注释去改 PSC/ARR。**

### ★ 已知待测：TIM1 的更新中断频率未必等于 PWM 频率

中心对齐模式下若 `RCR=0`（Legacy 正是 `TIM_RepetitionCounter = 0`），计数器上溢和下溢**各产生一次更新事件**，中断频率可能是 PWM 频率的 2 倍。

- **V1 不改 RCR，保持 0**（与 Legacy 一致）
- [ ] ★ 实测：在 FOC ISR 里翻转一个空闲 GPIO，示波器读三个数——**PWM 基波频率 / 更新中断频率 / FOC 调度频率**，分别记进第 13 节
- [ ] ★ 测出多少记多少，不要为了凑"10 kHz"去改配置

（中心对齐一个完整周期是 `0→ARR→0` = 2×ARR 个计数，ARR=99 → 2 MHz ÷ 198 ≈ 10.10 kHz，也不是精确 10.00 kHz。）

---

## 3. 引脚分配（19 个）

| # | 引脚 | CubeMX Signal | 板卡网络 | Legacy 出处 |
|---|---|---|---|---|
| 1 | PH0-OSC_IN | RCC_OSC_IN | 8 MHz 晶振 | `system_stm32f4xx.c` |
| 2 | PH1-OSC_OUT | RCC_OSC_OUT | | |
| 3 | PA8 | TIM1_CH1 | PM1_PWM_UH | `time1.c` AF_TIM1 |
| 4 | PA9 | TIM1_CH2 | PM1_PWM_VH | 同上 |
| 5 | PA10 | TIM1_CH3 | PM1_PWM_WH | 同上 |
| 6 | PB13 | TIM1_CH1N | PM1_PWM_UL | 同上 |
| 7 | PB14 | TIM1_CH2N | PM1_PWM_VL | 同上 |
| 8 | PB15 | TIM1_CH3N | PM1_PWM_WL | 同上 |
| 9 | PC6 | TIM3_CH1 | PM1_ENCA | `encoderini.c` |
| 10 | PC7 | TIM3_CH2 | PM1_ENCB | 同上 |
| 11 | PA3 | ADC3_IN3 | PM1_AMPW | `adc.c` |
| 12 | PA6 | ADC2_IN6 | PM1_AMPV | 同上 |
| 13 | PB1 | ADC1_IN9 | PM1_VBUS | 同上（**注意不是 PB0**） |
| 14 | PF10 | GPIO_Output | PM1_CTRL_SD | `time1.c:29` |
| 15 | PB12 | GPIO_Input | TIM1_BKIN / 刹车 | `time1.c:43-46` |
| 16 | PB6 | USART1_TX | 串口1-TX（CH340） | **见第 10 节 D-01** |
| 17 | PB7 | USART1_RX | 串口1-RX（CH340） | 同上 |
| 18 | PA13 | SYS_JTMS-SWDIO | SWD | |
| 19 | PA14 | SYS_JTCK-SWCLK | SWD | |

> **不要占用 PA15 / PB3**：IO 表显示这两脚是 `PM2_ENCA/ENCB` = `TIM2_CH1/CH2`。V1 的 TIM2 只当纯时基，不配通道。

已核对：Legacy `adc.c` 用 `GPIO_Pin_1` + `ADC_Channel_9`（自洽 = PB1），而项目认知文档写的 `PB0` 是错的——IO 表确认 **PB0 = PM1_AMPU（U 相电流）**、**PB1 = PM1_VBUS**。

---

## 4. ★ TIM1 逐项对照（SPL → HAL）

Legacy 出处：`Legacy/SYSTEM/time1.c` 的 `TIM1_Int_Init(100-1, 84-1)`。
**生成代码后，把 `MX_TIM1_Init()` 逐行对着这张表核。**

### 4.1 时基

| Legacy SPL | 值 | HAL 对应 |
|---|---|---|
| `TIM_Period` | 99 | `htim1.Init.Period = 99` |
| `TIM_Prescaler` | 83 | `htim1.Init.Prescaler = 83` |
| `TIM_CounterMode_CenterAligned1` | | `htim1.Init.CounterMode = TIM_COUNTERMODE_CENTERALIGNED1` |
| `TIM_ClockDivision = TIM_CKD_DIV1` | | `htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1` |
| `TIM_RepetitionCounter` | **0** | `htim1.Init.RepetitionCounter = 0` |
| `TIM_ARRPreloadConfig(ENABLE)` | | `htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE` |

### 4.2 输出比较（CH1/CH2/CH3 三组相同）

| Legacy SPL | 值 | HAL 对应 |
|---|---|---|
| `TIM_OCMode_PWM1` | | `sConfigOC.OCMode = TIM_OCMODE_PWM1` |
| `TIM_OutputState_Enable` | | 通道模式 = `PWM Generation CHx CHxN` |
| `TIM_OutputNState_Enable` | | 运行时 `HAL_TIMEx_PWMN_Start()` |
| `TIM_OCPolarity_High` | | `sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH` |
| `TIM_OCNPolarity_High` | | `sConfigOC.OCNPolarity = TIM_OCNPOLARITY_HIGH` |
| `TIM_OCIdleState_Reset` | | `sConfigOC.OCIdleState = TIM_OCIDLESTATE_RESET` |
| `TIM_OCNIdleState_Reset` | | `sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET` |
| `TIM_OCPreload_Enable` | | `sConfigOC.OCFastMode = TIM_OCFAST_DISABLE`（预装载 HAL 默认使能） |

> Legacy 没有设置 `TIM_Pulse`，即 CCR 初值为 0 → 上电占空比 0%。`.ioc` 里我填的是 `Pulse=50`，**这是与 Legacy 不一致的一处**，见第 10 节 D-02。

### 4.3 刹车与死区（BDTR）★ 本表最关键的一块

| Legacy SPL | 值 | HAL 对应 |
|---|---|---|
| `TIM_OSSRState_Disable` | | `sBreakDeadTimeConfig.OffStateRunMode = TIM_OSSR_DISABLE` |
| `TIM_OSSIState_Disable` | | `sBreakDeadTimeConfig.OffStateIDLEMode = TIM_OSSI_DISABLE` |
| `TIM_LOCKLevel_1` | | `sBreakDeadTimeConfig.LockLevel = TIM_LOCKLEVEL_1` |
| `TIM_DeadTime` | **84** | `sBreakDeadTimeConfig.DeadTime = 84` |
| `TIM_Break_Enable` | | `sBreakDeadTimeConfig.BreakState = TIM_BREAK_ENABLE` |
| `TIM_BreakPolarity_High` | | `sBreakDeadTimeConfig.BreakPolarity = TIM_BREAKPOLARITY_HIGH` |
| `TIM_AutomaticOutput_Enable` | | `sBreakDeadTimeConfig.AutomaticOutput = TIM_AUTOMATICOUTPUT_ENABLE` |

> **V1 按寄存器原样复刻，包括 `Break_Enable`。**
> Legacy 的这一组合是"BDTR 声明了 Break，但 PB12 没配 AF，所以 BKIN 进不了定时器"——**V1 照原样重现这个状态，不在迁移阶段改成 Disable**。
> 这属于设计意图与硬件行为不一致，登记为第 10 节 D-03，由 V2 处理。
>
> `TIM_DeadTime = 84` 是 **BDTR.DTG 的编码值**，不等于 84 ns。CKD 分频与 DTG 编码规则要一起看，实际死区时间以波形实测为准。

### 4.4 触发与使能

| Legacy SPL | HAL 对应 |
|---|---|
| `TIM_SelectOutputTrigger(TIM1, TIM_TRGOSource_OC1Ref)` | `sMasterConfig.MasterOutputTrigger = TIM_TRGO_OC1REF` |
| `TIM_CtrlPWMOutputs(TIM1, ENABLE)` | `HAL_TIMEx_PWMN_Start()` / `__HAL_TIM_MOE_ENABLE()` |
| `TIM_ITConfig(TIM1, TIM_IT_Update, ENABLE)` | `HAL_TIM_Base_Start_IT(&htim1)` |
| `TIM_SetCompare1/2/3(TIM1, tcm1/2/3)` | `__HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_n, ...)` |

> 注意：ADC 的外部触发用的是 **`ADC_ExternalTrigConv_T1_CC1`**（CC1 比较事件），不是 TRGO。TRGO=OC1Ref 是另一条独立通路。

### 4.5 CubeMX 侧要勾的项

- [ ] Clock Source：Internal Clock
- [ ] Channel1/2/3：**PWM Generation CH1 CH1N / CH2 CH2N / CH3 CH3N** ★（手写 `.ioc` 最不确定处）
- [ ] Channel4：Disable
- [ ] Prescaler 83 / Counter Mode **Center Aligned Mode 1** / Period 99 / RCR **0** / ARPE Enable
- [ ] Pulse：**改为 0**（对齐 Legacy，见 D-02）
- [ ] Dead Time 84 / Lock **Level 1** / OSSR Disable / OSSI Disable
- [ ] **Break State：Enable** ★（对齐 Legacy，由 V2 处理）
- [ ] Break Polarity：High / Automatic Output：Enable
- [ ] NVIC：TIM1_UP_TIM10 使能，抢占 0 / 子 0

---

## 5. TIM2 / TIM4 逐项对照（通用定时器时基）

Legacy：`Legacy/USER/time2.c` 的 `Time2_Init(999, 83)`。

| Legacy SPL | 值 | HAL 对应 |
|---|---|---|
| `TIM_CounterMode_Up` | | `htim2.Init.CounterMode = TIM_COUNTERMODE_UP` |
| `TIM_Period` | 999 | `htim2.Init.Period = 999` |
| `TIM_Prescaler` | 83 | `htim2.Init.Prescaler = 83` |
| `TIM_ClockDivision = TIM_CKD_DIV1` | | `TIM_CLOCKDIVISION_DIV1` |
| `TIM_RepetitionCounter` | 0 | `0` |
| `TIM_InternalClockConfig` | | Clock Source = Internal Clock |
| `TIM_SetCounter(TIM2, 0)` | | `__HAL_TIM_SET_COUNTER(&htim2, 0)` |
| `TIM_ITConfig(TIM2, Update, ENABLE)` | | `HAL_TIM_Base_Start_IT(&htim2)` |

- [ ] 所有 Channel 保持 Disable
- [ ] NVIC：TIM2 使能，抢占 1 / 子 1
- [ ] **auto-reload preload = Disable**（D-07：Legacy 未调用 `TIM_ARRPreloadConfig`）
- [ ] 84 MHz ÷ (83+1) = 1 MHz；ARR=999 → 更新周期 1 ms ✓

### TIM4 逐项对照

Legacy：`Legacy/USER/time2.c:61` 的 `Time4_Init(speed_period_max, 10)`，唯一调用点 `Legacy/USER/main.c:57`。

| Legacy SPL（`time2.c` 行号） | 值 | HAL 对应 |
|---|---|---|
| `TIM_Period = arr`（:69） | **10000** ← 见下方溯源 | `htim4.Init.Period = 10000` |
| `TIM_Prescaler = psc`（:70） | 10 | `htim4.Init.Prescaler = 10` |
| `TIM_CounterMode_Up`（:68） | | `TIM_COUNTERMODE_UP` |
| `TIM_ClockDivision = TIM_CKD_DIV1`（:67） | | `TIM_CLOCKDIVISION_DIV1` |
| `TIM_RepetitionCounter`（:71） | 0 | `0` |
| `TIM_SetCounter(TIM4, 0)`（:74） | | `__HAL_TIM_SET_COUNTER(&htim4, 0)` |
| `TIM_ITConfig(TIM4, Update, ENABLE)`（:76） | | `HAL_TIM_Base_Start_IT(&htim4)` |
| `TIM_Cmd(TIM4, ENABLE)`（:82） | | 同上 |
| NVIC `TIM4_IRQn`（:77-81） | 抢占 1 / 子 1 | 见第 9 节 |
| **未调用 `TIM_ARRPreloadConfig`** | ARPE = **0（关闭）** ★ | CubeMX 默认 Enable，需改成 Disable |

- [ ] 所有 Channel 保持 Disable（Legacy 未配置任何 TIM4 通道/引脚）
- [ ] NVIC：TIM4 使能，抢占 **1** / 子 **1**（与 TIM2 相同）

**★ ARR = 10000 的溯源（并更正我先前的一处错误）**

`speed_period_max` 是全局量，`main.c:26` 定义为 `long speed_period_min=10000, speed_period_max=10000;`。

**调用顺序才是关键：**

```
main.c:57   Time4_Init(speed_period_max, 10);      ← 此时 speed_period_max = 10000
main.c:60   cul_Saccdec();                         ← 之后才把它改成 26715
main.c:328  speed_period_max = DEC_ACC_PERiod[0];  ← 死写入，此后无人读取
```

`Time4_Init()` 在整个工程里**只被调用这一次**，之后再不调用。所以 **TIM4.ARR 被锁存为 10000**，`cul_Saccdec()` 算出的 26715 从来没送达 TIM4。

> ⚠️ 我先前在本表写过"在 `cul_Saccdec()` **之后**调用"，并据此复算出 26715 填进 `.ioc` —— **调用顺序判反了**。已按源码更正为 10000。
> 教训：复算过程再精确，前提（调用顺序）错了结果就是错的。**读源码 > 推演。**

**副产物**：`cul_Saccdec()` 的输出去向只有 `speed_period_min`、`dec_len`、`DEC_ACC_PERiod[]`，全部只在插补模块内部被消费；`speed_period_max` 那次赋值是死写入。
**所以 TIM4 与 S 曲线完全解耦** —— V1 把 ARR 冻结成常量 10000，与 Legacy 行为等价。

TIM4 时钟 84 MHz ÷ (10+1) = 7.636 MHz；ARR=10000 → 更新周期 **≈ 1.31 ms**。

- [ ] ★ 生成代码后确认 `htim4.Init.Period = 10000`、`Prescaler = 10`、**ARPE = Disable**

**★ 一个必须处理的陷阱**

`Time4_Init()` 打开了 TIM4 更新中断，其消费者是 `TIM4_IRQHandler`（`main.c` 里 `if(motorX_pluse==1){motorX_pluse=0;motorX_position++;}`）。

CubeMX 会生成 `TIM4_IRQHandler` → `HAL_TIM_IRQHandler` → `HAL_TIM_PeriodElapsedCallback`。**我必须在用户层补上 TIM4 的 callback 分支**，否则：
- 若 NVIC 使能了中断但回调里没有 TIM4 分支 → 中断被清除但什么都不做（无害，但 TIM4 变成纯空转）
- 若连 `HAL_TIM_PeriodElapsedCallback` 都没写 → CubeMX 的 weak 默认空实现仍会被调用（HAL 内部处理），不会死循环，但同样空转

**关键是不能出现"NVIC 使能了、cube 没生成 handler、也没人清标志"的组合** —— 那会反复进中断。

- [ ] 在用户层 `HAL_TIM_PeriodElapsedCallback()` 里补 TIM4 分支，行为照抄 `TIM4_IRQHandler`
- [ ] `motorX_pluse` / `motorY_pluse` 保持为 0（插补不接入），回调实际是 no-op —— 这正是"外设保留、算法不接入"的预期状态

---

## 6. TIM3 逐项对照

Legacy：`Legacy/USER/encoderini.c` 的 `Encoder_Int_Init(0xFFFF, 0)`。

| Legacy SPL | 值 | HAL 对应 |
|---|---|---|
| `TIM_Period` | 65535 | `htim3.Init.Period = 65535` |
| `TIM_Prescaler` | 0 | `htim3.Init.Prescaler = 0` |
| `TIM_CounterMode_Up` | | `TIM_COUNTERMODE_UP` |
| `TIM_EncoderInterfaceConfig(TIM_EncoderMode_TI12, Rising, Rising)` | | Combined Channels = **Encoder Mode**；Encoder Mode = **Encoder Mode TI1 and TI2** ★ |
| `TIM_ICFilter = 4`（CH1、CH2） | | `sConfig.IC1Filter = 4` / `IC2Filter = 4` |
| `TIM_ICPSC_DIV1` | | `IC1Prescaler = TIM_ICPSC_DIV1` |
| `TIM_ICSelection_DirectTI`（两通道） | | CubeMX 选编码器模式后自动处理 |
| `TIM_SetCounter(TIM3, 0)` | | `__HAL_TIM_SET_COUNTER(&htim3, 0)` |
| `TIM_ITConfig(TIM3, Update, ENABLE)` | | `HAL_TIM_Encoder_Start_IT(&htim3, TIM_CHANNEL_ALL)` |
| `TIM_GetCounter(TIM3)` | | `__HAL_TIM_GET_COUNTER(&htim3)` |
| `TIM3->CR1 & TIM_CR1_DIR`（判方向） | | `__HAL_TIM_IS_TIM_COUNTING_DOWN(&htim3)` |

- [ ] NVIC：TIM3 使能，抢占 1 / 子 0
- [ ] **auto-reload preload = Disable**（D-07：Legacy 未调用 `TIM_ARRPreloadConfig`）
- [ ] 编码器计数由输入边沿驱动，不是 84 MHz 自由计数

---

## 7. ADC1/2/3 逐项对照

Legacy：`Legacy/SYSTEM/adc.c` 的 `ADC_Int_Init()`。

| Legacy SPL | 值 | HAL / CubeMX 对应 |
|---|---|---|
| `ADC_Mode_Independent` | | 各 ADC 独立（不开多重模式） |
| `ADC_Prescaler_Div4` | | Clock Prescaler = **Synchronous clock mode divided by 4** ★（84÷4 = 21 MHz < 36 MHz） |
| `ADC_TwoSamplingDelay_5Cycles` | | 仅多重模式有效，独立模式下无影响 |
| `ADC_DMAAccessMode_Disabled` | | DMA 不启用 |
| `ADC_Resolution_12b` | | Resolution = 12 bits |
| `ADC_DataAlign_Right` | | Right alignment |
| `ADC_ScanConvMode = DISABLE` | | Scan Conversion Mode = Disabled |
| `ADC_ContinuousConvMode = ENABLE` | | Continuous Conversion Mode = **Enabled** |
| `ADC_NbrOfConversion = 1` | | Number Of Conversion = 1 |
| `ADC_ExternalTrigConvEdge_Falling` | | Trigger detection on **falling edge** ★ |
| `ADC_ExternalTrigConv_T1_CC1` | | External Trigger = **Timer 1 Capture Compare 1 event** ★ |
| `ADC_SampleTime_480Cycles` | | Rank1 Sampling Time = **480 Cycles** |
| `ADC_Cmd(ADCx, ENABLE)`（三个） | | `HAL_ADC_Start(&hadc1/2/3)` |
| 通道：ADC3→CH3(PA3) / ADC2→CH6(PA6) / ADC1→CH9(PB1) | | 见第 3 节 |

**V1 的取值方式也照抄 Legacy**：`adc.c` 是在 `main()` 和 `test()` 里轮询 EOC 取值（`ADC_GetValue` 内部 `while(EOC==RESET);`），而且 `ADC_ProcessData()` 连调 6 次（每个 ADC 读两遍、用第二遍）。

- [ ] V1 保持同样的"轮询取最后一次"行为，用 `HAL_ADC_PollForConversion()` + `HAL_ADC_GetValue()` 等价实现
- [ ] **不在 V1 改成注入组/DMA**——那是 V2

> ★ V1 必须实测的一件事：`ADC_ProcessData()` 的阻塞耗时（6 次轮询，量级可能 70~140 µs）。它同时被 `main()` 和 FOC 中断调用。
> 把耗时和 ISR 周期一起测出来，判断是否溢出。**测出来是什么就是什么，V1 不改架构。**

---

## 8. GPIO 逐项对照

**`SYSTEM/time1.c`**

| 引脚 | Legacy 配置 | CubeMX（`.ioc` 实际值） |
|---|---|---|
| PF10 | OUT / PP / **下拉** / **50 MHz** / `GPIO_SetBits` 置高 | Output PP，`GPIO_PUPD_DOWN`，`GPIO_SPEED_FREQ_HIGH`，初始电平 **High** |
| PA8/9/10 | AF / PP / 50 MHz / 下拉（继承）/ `AF_TIM1` | TIM1_CH1/2/3，`GPIO_MODE_AF_PP` / `GPIO_PUPD_DOWN` / `GPIO_SPEED_FREQ_HIGH` |
| PB13/14/15 | AF / PP / 50 MHz / 下拉（继承）/ `AF_TIM1` | TIM1_CH1N/2N/3N，同上 |
| PB12 | **IN / NOPULL**（普通 GPIO，非 AF） | Input，`GPIO_PUPD_NOPULL`，**不勾 EXTI、不开中断** |

> `GPIO_Speed_50MHz`（SPL）在 F4 HAL 里对应 **`GPIO_SPEED_FREQ_HIGH`**（F4 的 OSPEEDR 编码：00=2M / 01=25M / 10=50M / 11=100M），**不是** `VERY_HIGH`。

**`USER/encoderini.c`**

| 引脚 | Legacy 配置 | CubeMX（`.ioc` 实际值） |
|---|---|---|
| PC6 / PC7 | AF / 50 MHz / NOPULL / `AF_TIM3` | TIM3_CH1 / CH2，`GPIO_MODE_AF_PP` / `GPIO_PUPD_NOPULL` / `GPIO_SPEED_FREQ_HIGH` |

**`SYSTEM/adc.c`**

| 引脚 | Legacy 配置 | CubeMX（`.ioc` 实际值） |
|---|---|---|
| PA3 / PA6 / PB1 | `GPIO_Mode_AN` / NOPULL | ADC3_IN3 / ADC2_IN6 / ADC1_IN9，`GPIO_MODE_ANALOG` / `GPIO_PUPD_NOPULL` |

**新增引脚（无 Legacy 参照，见 D-01）**

| 引脚 | CubeMX（`.ioc` 实际值） | 依据 |
|---|---|---|
| PB6 / PB7 | `GPIO_MODE_AF_PP` / `GPIO_PUPD_NOPULL` / `GPIO_SPEED_FREQ_VERY_HIGH` | CubeMX USART 默认值。Legacy 那份 usart.c 整文件被注释、从未生效，**不存在可对标的 Legacy GPIO 配置** |

> **`.ioc` 的 GPIO_Speed 一栏全部已显式写出**，不再依赖 CubeMX 默认值 —— 原先漏写导致 PF10 等多处与 Legacy 不等价。

---

## 9. NVIC

| IRQ | 抢占 / 子 | Legacy 出处 |
|---|---|---|
| `TIM1_UP_TIM10_IRQn` | 0 / 0 | `time1.c` |
| `TIM3_IRQn` | 1 / 0 | `encoderini.c` |
| `TIM2_IRQn` | 1 / 1 | `time2.c` |
| `TIM4_IRQn` | 1 / 1 | `time2.c` |
| `SysTick_IRQn` | 15 / 0 | CubeMX 默认 |

- [ ] NVIC Priority Group：**`NVIC_PRIORITYGROUP_4`**

> ★ GROUP4 下 4 位全给抢占、子优先级 0 位，所以 **TIM3 的 (1,0) 和 TIM2 的 (1,1) 在硬件上是同一个抢占等级**，子优先级不起作用。
> Legacy 根本没调用过 `NVIC_PriorityGroupConfig`，原本的语义就不明确。V1 照记，不重新分层。

---

## 10. V1 差异登记表

> 格式：`Legacy 原配置 → 问题 → HAL 等价配置 → 是否修复(V1) → 修复依据`
> **V1 阶段"是否修复"原则上一律填「否」**，逐条在 V2 里处理。

| 编号 | Legacy 原配置 | 问题 | HAL 等价配置 | V1 是否修复 | 依据 |
|---|---|---|---|---|---|
| **D-01** | `usart/usart.c` 想用 PA9/PA10 做 USART1（且整文件被注释，从未生效） | PA9/PA10 = TIM1_CH2/CH3，**物理冲突** | 改用 **PB6/PB7** | **是** | 证据优先级：IO 表 > Legacy 代码。IO 表确认 PB6/PB7 = 串口1，默认经 JP7 接板载 CH340。Legacy 那份 usart.c 本身没生效，不存在"行为等价"对象 |
| **D-02** | TIM1 三个 `TIM_OCInit` 都没设 `TIM_Pulse`，CCR 初值 = 0 | 上电占空比 0% | `.ioc` 我误填 `Pulse=50` | **是**（改为 0） | 对齐 Legacy 寄存器值 |
| **D-03** | BDTR 开 `Break_Enable` + `BreakPolarity_High` + `AutomaticOutput_Enable`，但 PB12 是普通 GPIO 输入**非 AF** | 声明了硬件 Break 但 BKIN 进不了定时器，**实际不生效** | 寄存器原样复刻（Break = Enable，PB12 = GPIO_Input） | **否** | 用户原则：V1 重现 Legacy 实际行为，整改单列。→ V2-SF-01 |
| **D-04** | `time1.c` 复用 `GPIO_InitStructure`：PF10 先设 `PuPd_DOWN` / `50 MHz`，随后 PA8-10、PB13-15 复用同一结构体，**继承了下拉和 50 MHz** | 我先前把 AF 引脚和 PF10 都配成 `NOPULL` / `FREQ_LOW`，与 Legacy 不等价，且与本表第 8 节自相矛盾 | **全部按 Legacy 恢复**：PF10 / PA8-10 / PB13-15 → `GPIO_PUPD_DOWN` + `GPIO_SPEED_FREQ_HIGH` | **否**（恢复等价） | 放弃先前"副作用可忽略"的判断（用户审计判定 FAIL，成立）。CubeMX 完全能逐引脚设 pull，"无法继承"站不住脚；且下拉在复位期间让门极驱动输入保持低电平，反而偏安全 |
| **D-05** | `Time4_Init(speed_period_max, 10)` 配置 TIM4（`time2.c:61`，唯一调用点 `main.c:57`） | 我先前误判调用顺序，把 ARR 算成 26715 | **V1 按 Legacy 配置 TIM4**：PSC=10、ARR=**10000**（`main.c:26` 初值，在 `main.c:57` 被锁存）、更新中断 + NVIC 1/1 | **否**（保留） | 用户拍板：**V1 可以换机制，不能擅自删 Legacy 已存在的外设和调度关系**。`cul_Saccdec()` 在 `main.c:60` 才执行，其算出值从未送达 TIM4，故 TIM4 与 S 曲线解耦，冻结 ARR=10000 即为等价 |
| **D-06** | `SystemInit()` 里 `SetSysClock()` 配置 PLL | — | CubeMX `SystemClock_Config()` | 是 | 机制替换，参数保持一致（第 2 节） |
| **D-07** | TIM2/TIM3/TIM4 均**未**调用 `TIM_ARRPreloadConfig`，ARPE 保持复位值 **0（关闭）** | CubeMX 的 "auto-reload preload" 默认是 **Enable** | 三者的 ARPE 都设成 **Disable** | **是** ★ | 寄存器级等价。V1 中三者 ARR 都不变，此项功能上是惰性的，但属寄存器差异，需在 GUI 里确认 |
| **D-08** | Legacy 未定义的项：`.ioc` 中 `TIM1.MasterOutputTrigger` / `MasterSlaveMode` 的**键名字符串未经验证** | 我按 HAL 常量名推测（`TIM_TRGO_OC1REF`、`TIM_MASTERSLAVEMODE_DISABLE`），无法离线确认 CubeMX 是否识别 | 保留推测值，**但必须在 GUI 里核对 Trigger Output (TRGO) 是否真的显示 `OC1REF`** | **待 GUI 确认** ★ | 与 D-02 的 `Pulse` 同类风险：手写 `.ioc` 的键名/取值可能不被识别。疑似不被识别时，直接在 GUI 下拉框里设，以 GUI 保存结果为准 |

- [ ] 新增差异一律登记进本表，不进正文

> **本表当前状态**
> - D-01 ~ D-07：已定，无待确认项
> - D-08：**待 GUI 确认**（`TIM1.MasterOutputTrigger` / `MasterSlaveMode` 的键名是我推测的，需在 CubeMX 里核对 TRGO 是否显示为 `OC1REF`）
> - 用户二次审计已闭合的 3 项：ADC2/ADC3 补全七项参数、TIM1 TRGO 已编码、PF10 改回 `PUPD_DOWN` + `FREQ_HIGH`

---

## 11. 生成前最后确认

- [ ] 引脚视图 19 个引脚全绿，无冲突提示
- [ ] Clock Configuration HCLK = **168 MHz**
- [ ] Toolchain = CMake
- [ ] 第 4/5/6/7 节的 ★ 项在 GUI 里确认过

---

## 12. 生成后自检

- [ ] 生成：`Core/`、`Drivers/`、`CMakeLists.txt`、`CMakePresets.json`、`cmake/`、`startup_stm32f407xx.s`、`STM32F407IGTx_FLASH.ld`
- [ ] `MX_GPIO_Init` / `MX_ADC1_Init` / `MX_ADC2_Init` / `MX_ADC3_Init` / `MX_TIM1_Init` / `MX_TIM2_Init` / `MX_TIM3_Init` / `MX_TIM4_Init` / `MX_USART1_UART_Init` 都在
- [ ] `SystemClock_Config()`：PLLM=8 / PLLN=336 / PLLP=RCC_PLLP_DIV2 / PLLQ=7，HCLK=**168000000**
- [ ] ★ **`MX_TIM1_Init()` 逐行对照第 4.1~4.4 节表格**，重点确认 BDTR 七项和 `sMasterConfig`
- [ ] ★ `MX_TIM2_Init()` / `MX_TIM3_Init()` 对照第 5/6 节
- [ ] `cmake/gcc-arm-none-eabi.cmake` 的 `TARGET_FLAGS` 含 `-mcpu=cortex-m4` + FPU 标志（`-mfpu=fpv4-sp-d16 -mfloat-abi=hard`）★
- [ ] 先跑 `cmake --list-presets`，不要直接套用其他工程的 preset 名

```bash
cmake --list-presets
cmake --preset <实际存在的preset>
cmake --build build/<preset>
```

- [ ] 0 error，`.elf` / `.map` / `.bin` 实际生成

---

## 13. 实测记录（V1 允许的观测，不得编造）

**V1 只做无功率观测。**驱动器功率直流侧**不通电**。

| # | 项目 | 预期/依据 | 实测 | 判定 |
|---|---|---|---|---|
| 01 | CMake 构建 | 0 error | | |
| 02 | SWD 单步/断点 | 可用 | | |
| 03 | 串口输出 | 115200 8N1 有输出 | | |
| 04 | GPIO 默认电平 | PF10 = 高（Legacy 行为） | | |
| 05 | PWM 基波频率 | 配置推算 ≈10.1 kHz | | |
| 06 | **TIM1 更新中断频率** | ★ 待测，可能是 2×PWM | | |
| 07 | FOC 调度频率 | ★ 待测 | | |
| 08 | Timer 时钟 | TIM1=168M / TIM2=TIM3=84M | | |
| 09 | 三相六路波形 | 互补、死区可见 | | |
| 10 | `ADC_ProcessData()` 阻塞耗时 | ★ 待测 | | |

---

## 14. 移交 V2

以下**全部不在 V1 范围**，见 [`V2_硬件安全修正清单.md`](V2_硬件安全修正清单.md)：

- BKIN 是否真正接通、PF10/CTRL_SD 极性实测、R70 锁存
- ADC 从轮询改为 PWM 指定相位触发注入组
- 启动/停机顺序、故障状态机、限流保护
- RCR 是否改为 1
- TIM2 与 PM2 编码器的资源冲突（双轴）
- ISR 从 HAL 回调改为轻量路径