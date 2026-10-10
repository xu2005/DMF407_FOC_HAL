# V1 · Legacy → HAL 映射表

> 用途：V1 迁移的 SPL → HAL API 与初值对照基准。
> **配置取值以 [`DMF407_HAL_引脚时钟配置规范.md`](DMF407_HAL_引脚时钟配置规范.md) 为唯一权威入口**，本文只负责"旧语句 → 新语句"与"初值对照"。
> 证据优先级见规范 §1.1；文件级纪律见任务书 §11。

- 目标工程：`DMF407_FOC_HAL`
- Legacy 归档：`Legacy/`
- **V1 唯一行为基线（已裁决）：`Legacy/Template222_A/`** —— `enable_flag=1`、`Speed_Kp=0.08f`、`Speed_Ki=0.0015f`；`Template222-100/` 仅作历史参考，不参与参数裁决（见 `../Legacy/README.md`）
- 生成后必须逐行核对 `Core/Src/main.c` 的 `MX_*_Init()`，**GUI / 生成代码是最终证据**（任务书 §5）

---

## 1. API 替换表

### 1.1 PWM 比较值

| Legacy SPL | HAL |
|---|---|
| `TIM_SetCompare1(TIM1, x)` | `__HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, x)` |
| `TIM_SetCompare2(TIM1, x)` | `__HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, x)` |
| `TIM_SetCompare3(TIM1, x)` | `__HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_3, x)` |

> 收口要求：`Control` / `Motor` 层**不得**散落直接操作 TIM1 CCR，统一经 BSP（任务书 §4.3）。建议形态：
> ```c
> void BSP_PWM_SetABC(uint16_t u, uint16_t v, uint16_t w);
> ```

### 1.2 计数器 / 方向

| Legacy SPL | HAL |
|---|---|
| `TIM_GetCounter(TIM3)` | `__HAL_TIM_GET_COUNTER(&htim3)` |
| `TIM3->CR1 & TIM_CR1_DIR` | `__HAL_TIM_IS_TIM_COUNTING_DOWN(&htim3)` |
| `TIM_SetCounter(TIMx, 0)` | `__HAL_TIM_SET_COUNTER(&htimx, 0)` |
| `TIM4->ARR` / `TIM4->CNT` | `__HAL_TIM_GET_AUTORELOAD(&htim4)` / `__HAL_TIM_GET_COUNTER(&htim4)` |

### 1.3 启动与中断

| Legacy SPL | HAL |
|---|---|
| `TIM_Cmd(TIMx, ENABLE)` + `TIM_ITConfig(TIMx, TIM_IT_Update, ENABLE)` | `HAL_TIM_Base_Start_IT(&htimx)` |
| `TIM_Cmd(TIMx, ENABLE)`（无中断） | `HAL_TIM_Base_Start(&htimx)` |
| 编码器（含更新中断） | `HAL_TIM_Encoder_Start_IT(&htim3, TIM_CHANNEL_ALL)` |
| `TIM_CtrlPWMOutputs(TIM1, ENABLE)` | `HAL_TIMEx_PWMN_Start()` + `__HAL_TIM_MOE_ENABLE(&htim1)` |
| `TIM_ClearITPendingBit` / `TIM_GetITStatus` | `HAL_TIM_IRQHandler()` → `HAL_TIM_PeriodElapsedCallback()` |

### 1.4 GPIO

| Legacy SPL | HAL |
|---|---|
| `GPIO_Init()` | `HAL_GPIO_Init()` |
| `GPIO_SetBits(PORT, PIN)` | `HAL_GPIO_WritePin(PORT, PIN, GPIO_PIN_SET)` |
| `GPIO_ResetBits(PORT, PIN)` | `HAL_GPIO_WritePin(PORT, PIN, GPIO_PIN_RESET)` |
| `GPIO_PinAFConfig()` | `GPIO_InitStruct.Alternate` |
| `GPIO_Speed_50MHz` | `GPIO_SPEED_FREQ_HIGH`（F4 OSPEEDR：00=2M / 01=25M / **10=50M** / 11=100M） |

### 1.5 ADC（V1 保持阻塞读，不得改 Injected / DMA）

| Legacy SPL | HAL |
|---|---|
| `ADC_Cmd(ADCx, ENABLE)` | `HAL_ADC_Start(&hadcx)` |
| `while(ADC_GetFlagStatus(ADCx, ADC_FLAG_EOC) == RESET);` | `HAL_ADC_PollForConversion(&hadcx, timeout)` |
| `ADC_GetConversionValue(ADCx)` | `HAL_ADC_GetValue(&hadcx)` |
| `ADC_RegularChannelConfig(...)` | 由 `MX_ADCx_Init()` 的 Rank 配置承担 |

### 1.6 USART

| Legacy SPL | HAL |
|---|---|
| `USART_Init()` | `HAL_UART_Init()` |
| `USART_SendData()` | `HAL_UART_Transmit()` |

> 禁止在 FOC 高频 ISR 内阻塞打印（规范 §10）。

---

## 2. 初始化参数对照

### 2.1 TIM1（Legacy `SYSTEM/time1.c` · `TIM1_Int_Init(100-1, 84-1)`）

| Legacy SPL | 值 | 生成代码中应出现 |
|---|---|---|
| `TIM_Period` | 99 | `htim1.Init.Period = 99` |
| `TIM_Prescaler` | 83 | `htim1.Init.Prescaler = 83` |
| `TIM_CounterMode_CenterAligned1` | | `htim1.Init.CounterMode = TIM_COUNTERMODE_CENTERALIGNED1` |
| `TIM_CKD_DIV1` | | `htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1` |
| `TIM_RepetitionCounter` | **0** | `htim1.Init.RepetitionCounter = 0` |
| `TIM_ARRPreloadConfig(ENABLE)` | | `htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE` |
| `TIM_OCMode_PWM1` | | `sConfigOC.OCMode = TIM_OCMODE_PWM1` |
| `TIM_OutputState_Enable` / `OutputNState_Enable` | | 三通道 `PWM Generation CHx CHxN` |
| `TIM_OCPolarity_High` / `OCNPolarity_High` | | `TIM_OCPOLARITY_HIGH` / `TIM_OCNPOLARITY_HIGH` |
| `TIM_OCIdleState_Reset` / `OCNIdleState_Reset` | | `TIM_OCIDLESTATE_RESET` / `TIM_OCNIDLESTATE_RESET` |
| `TIM_OCPreload_Enable` ×3 | | `sConfigOC.OCFastMode = TIM_OCFAST_DISABLE` |
| **未设 `TIM_Pulse`** | **CCR=0** | `Pulse = 0`（三通道） |
| `TIM_OSSRState_Disable` / `OSSIState_Disable` | | `OffStateRunMode` / `OffStateIDLEMode = TIM_OSSR_DISABLE` / `TIM_OSSI_DISABLE` |
| `TIM_LOCKLevel_1` | | `sBreakDeadTimeConfig.LockLevel = TIM_LOCKLEVEL_1` |
| `TIM_DeadTime` | **84**（DTG 编码值，≠84 ns） | `DeadTime = 84` |
| `TIM_Break_Enable` / `BreakPolarity_High` / `AutomaticOutput_Enable` | | `BreakState = TIM_BREAK_ENABLE` 等三项 |
| `TIM_SelectOutputTrigger(TIM_TRGOSource_OC1Ref)` | | `sMasterConfig.MasterOutputTrigger = TIM_TRGO_OC1REF`<br>`sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE` |

### 2.2 TIM2 / TIM3 / TIM4

| 项 | TIM2 | TIM3 | TIM4 |
|---|---|---|---|
| Legacy 出处 | `USER/time2.c:7` `Time2_Init(999, 83)` | `USER/encoderini.c:6` `Encoder_Int_Init(0xFFFF, 0)` | `USER/time2.c:61` `Time4_Init(speed_period_max, 10)` |
| Clock | 84 MHz | 84 MHz | 84 MHz |
| Prescaler | 83 | 0 | **10** |
| Period | 999 | 65535 | **10000** |
| Counter Mode | Up | Up | Up |
| CKD | DIV1 | DIV1 | DIV1 |
| RCR | 0 | 0 | 0 |
| **ARPE** | **Disable**（Legacy 未调用 `TIM_ARRPreloadConfig`） | **Disable** | **Disable** |
| 通道 | 全部 Disable | Encoder TI1+TI2 | 全部 Disable |
| IC Filter | — | 4 / 4 | — |
| NVIC | 1 / 1 | 1 / 0 | 1 / 1 |

> **TIM4 ARR=10000 的溯源**：`main.c:26` 定义 `long speed_period_min=10000, speed_period_max=10000;`；`main.c:57` 调用 `Time4_Init(speed_period_max, 10)`；`main.c:60` 才调 `cul_Saccdec()`，其算出的 26715 **从未送达 TIM4**（`main.c:328` 那次赋值此后无人读取）。→ **禁止把 TIM4 ARR 改成 26715**（任务书 §9）。

### 2.3 ADC1 / ADC2 / ADC3

三个 ADC 参数**完全一致**（Legacy `SYSTEM/adc.c` 共用同一组 `ADC_InitStructure` 与 `ADC_CommonInitStructure`）：

| 项 | 值 |
|---|---|
| Clock Prescaler | PCLK2 / 4 |
| Resolution | 12 bit |
| Scan Conversion | Disable |
| Continuous Conversion | **Enable** |
| Data Alignment | Right |
| Nbr Of Conversion | 1 |
| External Trigger | **TIM1_CC1** |
| External Trigger Edge | **Falling** |
| Sampling Time | **480 Cycles** |
| DMA | Disabled |

通道：ADC1=IN9(PB1/VBUS)、ADC2=IN6(PA6/AMPV)、ADC3=IN3(PA3/AMPW)。

> `PB1 = PM1_VBUS`，`PB0 = PM1_AMPU ≠ VBUS`。**禁止把 PB1 改回 PB0**（任务书 §9）。

### 2.4 GPIO

| 引脚组 | Mode | Pull | Speed | Initial |
|---|---|---|---|---|
| PA8/9/10、PB13/14/15 | AF_PP（TIM1） | **PULLDOWN** | **50 MHz** | — |
| PC6 / PC7 | AF_PP（TIM3） | NOPULL | 50 MHz | — |
| PA3 / PA6 / PB1 | ANALOG | NOPULL | — | — |
| PF10 | Output PP | PULLDOWN | 50 MHz | **High** |
| PB12 | Input | NOPULL | — | 不配 AF、不开 EXTI |
| PB6 / PB7 | AF_PP（USART1） | NOPULL | Very High | — |

> 六路 PWM 的 PULLDOWN + 50 MHz 来自 `time1.c` 复用 `GPIO_InitStructure` 的**实际寄存器行为**，V1 按行为复刻（规范 §4.2）。**删除该行为必须登记**（任务书 §9）。

### 2.5 NVIC

| IRQ | 抢占 / 子 |
|---|---|
| TIM1_UP_TIM10 | 0 / 0 |
| TIM3 | 1 / 0 |
| TIM2 | 1 / 1 |
| TIM4 | 1 / 1 |
| SysTick | 15 / 0 |

Priority Group = `NVIC_PRIORITYGROUP_4`。GROUP4 下 `(1,0)` 与 `(1,1)` 的子优先级在硬件上不形成次级排序，V1 只记录不重设计。

### 2.6 ISR 路由

```c
TIMx_IRQHandler() → HAL_TIM_IRQHandler() → HAL_TIM_PeriodElapsedCallback()
```

| Instance | 动作（对照 Legacy） |
|---|---|
| TIM1 | FOC Tick（Legacy `USER/interru.c` → `test()`） |
| TIM2 | 角度/转速更新（Legacy `USER/time2.c` `TIM2_IRQHandler`） |
| TIM3 | 编码器溢出计数（Legacy `USER/angle.c` `TIM3_IRQHandler`） |
| TIM4 | Legacy `TIM4_IRQHandler`：`motorX_pluse`/`motorY_pluse` 置位则清零并自增位置 |

> TIM4 分支必须补上；V1 中 `motorX_pluse`/`motorY_pluse` 恒为 0，回调实际为 no-op。
> 业务逻辑不得堆在 `stm32f4xx_it.c`。高频 ISR 是否绕过 HAL 回调属 V2（任务书 §8）。

---

## 3. V1 差异登记表

格式：`Legacy 原配置 → 问题 → HAL 等价配置 → V1 是否修复 → 依据`

| 编号 | Legacy 原配置 | 问题 | HAL 等价配置 | V1 修复 | 依据 |
|---|---|---|---|---|---|
| **D-01** | `usart/usart.c` 想用 PA9/PA10 做 USART1（且整文件被注释，从未生效） | PA9/PA10 = TIM1_CH2/CH3，**物理冲突** | 改用 **PB6/PB7** | **是** | IO 表 > Legacy 代码。IO 表确认 PB6/PB7 = 串口1（JP7 接板载 CH340）。原 usart.c 未生效，无"行为等价"对象 |
| **D-02** | TIM1 三个 `TIM_OCInit` 均未设 `TIM_Pulse`，CCR 初值 0 | 上电占空比 0% | `Pulse = 0` | 否 | 对齐 Legacy 寄存器值。**禁止改成 50**（任务书 §9） |
| **D-03** | BDTR 开 `Break_Enable` + `BreakPolarity_High` + `AutomaticOutput_Enable`，但 PB12 是普通 GPIO 输入**非 AF** | 声明了硬件 Break 但 BKIN 进不了 TIM1，**实际不生效** | 寄存器原样复刻 | **否** | V1 复现 Legacy 实际行为，整改单列 → V2-SF-01。**禁止在 V1 关闭 Break 或把 PB12 改 AF** |
| **D-04** | `time1.c` 复用结构体，六路 PWM 继承 `PuPd_DOWN` + `50 MHz` | — | 按实际寄存器行为配 PULLDOWN + 50 MHz | 否 | 规范 §4.2 明确以"实际寄存器行为"为准 |
| **D-05** | `Time4_Init(speed_period_max, 10)`，唯一调用点 `main.c:57` | — | TIM4 保留：PSC=10、ARR=**10000**、IRQ 1/1 | 否 | 见 §2.2 溯源。**禁止删除 TIM4 或改成 26715** |
| **D-06** | `SystemInit()` 的 `SetSysClock()` | — | `SystemClock_Config()` | 是 | 机制替换，参数不变 |
| **D-07** | TIM2/3/4 均未调用 `TIM_ARRPreloadConfig`，ARPE 保持复位值 0 | CubeMX 默认 Enable | 三者 ARPE = **Disable** | 是 | 寄存器级等价。V1 中三者 ARR 不变，功能惰性 |
| **D-08** | TIM1 TRGO 未在手写 `.ioc` 中确认 | 手写键名不可靠 | GUI 设 `TRGO = OC1REF`、`MasterSlaveMode = Disable` | **✅ 已闭环** | 生成代码 `Core/Src/tim.c:68` 实测 `sMasterConfig.MasterOutputTrigger = TIM_TRGO_OC1REF;` |
| **D-09** | `ADC_GetValue()` 每次调用前都执行 `ADC_RegularChannelConfig(ADCx, ch, 1, 480Cycles)` 重配通道（`adc.c:73`） | HAL 侧无对应调用 | 通道只在 `MX_ADCx_Init()` 配置一次；运行时**不调用** `HAL_ADC_ConfigChannel()` | **是（有意省略）** | 通道已配成同值，运行时重配为 no-op；且该 HAL 函数对已使能 ADC 有使能状态副作用。属**代码级差异、非可观测行为差异** |
| **D-10** | `while (ADC_GetFlagStatus(ADCx, ADC_FLAG_EOC) == RESET);`（`adc.c:79`）**无限等待，无超时** | 可能永久阻塞 | `HAL_ADC_PollForConversion(hadc, HAL_MAX_DELAY)` | **否（保持等价）** | 已核实 HAL 源码 `stm32f4xx_hal_adc.c`：`Timeout == HAL_MAX_DELAY` 时整段跳过超时判断 → 真正的无限等待，语义等价。超时/故障处理归 V2 |
| **D-11** | `ADC_ProcessData()` 的调用点在 `main.c:67` 的**主循环**里（不在任何中断中） | V1 暂无消费者 | `BSP_ADC_ReadAll()` 已实现但**尚未接线** | **待接线** ★ | 接线时必须与 Legacy 一致放在**主循环**；**严禁放入 FOC ISR** —— 它是 6 次阻塞读，放中断内会破坏时序 |

---

## 4. 生成后核对清单

对 `Core/Src/main.c` 逐项核对，**必须贴出原始行作为证据**（任务书 §11）：

- [ ] `SystemClock_Config`：PLLM=8 / PLLN=336 / PLLP=DIV2 / PLLQ=7，HCLK=168000000
- [ ] `MX_TIM1_Init`：PSC=83、Period=99、CMS=CenterAligned1、RCR=0、ARPE=Enable、Pulse 三通道=0、BDTR 七项、`sMasterConfig` 两项
- [ ] `MX_TIM2_Init`：PSC=83、Period=999、ARPE=Disable
- [ ] `MX_TIM3_Init`：Encoder TI12、Period=65535、PSC=0、IC Filter=4、ARPE=Disable
- [ ] `MX_TIM4_Init`：PSC=10、Period=10000、ARPE=Disable
- [ ] `MX_ADC1/2/3_Init`：**三者参数完全一致**，仅通道不同
- [ ] `MX_GPIO_Init`：各引脚 Mode/Pull/Speed 与本表 §2.4 一致，PF10 初值 High
- [ ] `MX_USART1_UART_Init`：115200 / 8N1 / TX_RX
- [ ] NVIC：`HAL_NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_4)` + 四个 IRQ 优先级
- [ ] `stm32f4xx_it.c`：TIM1/TIM2/TIM3/TIM4 四个 Handler 均调用 `HAL_TIM_IRQHandler`

---

## 5. 相关文档

- [`DMF407_HAL_引脚时钟配置规范.md`](DMF407_HAL_引脚时钟配置规范.md) —— 配置唯一权威入口
- [`DMF407_HAL_CMake_模块化重构_Agent任务书.md`](DMF407_HAL_CMake_模块化重构_Agent任务书.md) —— 阶段与纪律
- [`V2_硬件安全修正清单.md`](V2_硬件安全修正清单.md) —— V2 层，A0~A4 未闭环前 A5/A6 = BLOCKED
- [`../Legacy/README.md`](../Legacy/README.md) —— **基线版本待裁决**
- `历史/CubeMX配置核对表.md` —— 已被本规范取代，仅作历史参考