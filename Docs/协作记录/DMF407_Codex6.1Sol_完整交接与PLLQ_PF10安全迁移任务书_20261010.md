# DMF407_FOC_HAL · V1 完整交接及 PLLQ / PF10 安全迁移执行任务书

> **交付目标**：Trae 中由 DeepSeek 向 **Codex 6.1 Sol** 原样交接，并由 Codex 在 Windows 本机实际执行。  
> **日期**：2026-10-10  
> **任务类型**：现有 STM32F407 工程持续改造、可复现构建与审计；**不是重新开发工程、不是仅输出建议或伪代码**。  
> **输入依据**：用户上传的《交接_2026-10-10_bsp_timer完成.md》、既有工程审计资料、用户在当前对话中的最新明确裁决。  
> **重要**：本文件本身是**待执行指令**，不是 PLLQ / PF10 已修改、ARM 已编译或已推送的验收报告。执行结果只能以本机实际源码、生成代码、编译日志和 Git 记录为准。

## 0. 给 Trae / DeepSeek 的转交指令

请把本 Markdown **完整、不删减地交给当前项目的 Codex 6.1 Sol**，使其在现有工作区执行。不要把任务仅改写为泛化建议，不要误认为用户要求重新建立 GitHub 仓库，也不要再次询问下述已经明确裁决的 A/B 问题。可由 DeepSeek 审核 Codex 的证据和差异，但实际文件修改、测试、Git 提交与推送由具有本机访问权限的 Codex 负责。

**用户已经正式裁决**：

1. **PLLQ：从当前 V1 的 4 修正为 7**，通过 STM32CubeMX 修改 `.ioc` 并重新 Generate Code；保持 SYSCLK、APB 定时器时钟和现有控制时序不变。
2. **PF10：选择原交接 §5.1 的 B（安全优先）**。HAL 工程的 `PM1_CTRL_SD` 默认进入**禁止输出态**；以原理图所示板级极性为依据，目标 `PF10=Low`。这是相对 `Template222_A` 的**显式安全例外**，需要单独提交和差异登记。
3. **功率级始终 `POWER-STAGE ENABLE BLOCKED`**。软件把 PF10 设为低电平，不等于物理安全验收完成，不允许通功率母线、带功率试车或运动负载实验。

上述三项是本任务的输入前提，不是等待 Agent 再投票的议题。若现场源码、原理图或板卡版本出现相反证据，应暂停**对应危险操作**并报告，不能擅自打开驱动使能。

---

## 1. 工程定位、路径与版本坐标

### 1.1 现有工程

```text
工程：DMF407_FOC_HAL
本机：C:\Users\xuhengyi\Desktop\item\课设\DMF407_FOC_HAL
远端：https://github.com/xu2005/DMF407_FOC_HAL
权限：Private
分支：main
硬件：STM32F407IGT6 + ATK-DMF407 + ATK-PD6010B
工具：STM32CubeMX + STM32 HAL + CMake + GNU Arm（非 Keil/SPL 新工程）
```

**核心目标**：以 Legacy 实际行为为基线，将 MCU 底层迁到 HAL/BSP，并在保留控制算法、参数和时间关系的前提下，形成可构建、可测试、可审计的工程。**绝不等于把 SPL 改写成任意新算法。**

### 1.2 版本纪律

用户上传的原交接文档记录：

- 截至交接正文：`fc320f7`；原文声称当时 `main=origin/main`。
- 此后本机 Agent 的续作记录还报告曾新增 `5ba7b11`，内容为交接文档入库。**该报告尚不等于当前 HEAD 的实时值。**

因此必须先运行以下命令，**以本机结果为准**，而不是强制退回上述任意一个提交：

```powershell
Set-Location 'C:\Users\xuhengyi\Desktop\item\课设\DMF407_FOC_HAL'
git rev-parse --short HEAD
git branch -vv
git log --oneline -12
git status -sb
git status --porcelain=v1
cmake --list-presets
cmake --build build/Debug
```

如有**非预期的未提交修改**，先列出文件与差异、确认来源；不得执行 `git reset --hard`、`git clean -fd` 或覆盖用户的新工作。已有 `patches/` 可保持未跟踪，不要把旧补丁当作本次任务重新套用。

### 1.3 交接阅读顺序（磁盘内容优先）

1. 本文，以及已有 `交接_2026-10-10_bsp_timer完成.md`。
2. `Docs/工作日志_2026-10-10.md`（如实际路径不同，先按文件名定位）。
3. `Docs/V1_复现验收框架.md`。
4. `Docs/V1_Legacy_Mapping.md`。
5. `DMF407_HAL_引脚时钟配置规范.md`（如在 Docs 或仓库根，使用实际路径）。
6. `Docs/V2_硬件安全修正清单.md` 及 R-01 / R-03、SF-02 相关登记。
7. `Legacy/Template222_A/`、`Legacy/Template222-100/` 与 `Core/`、`BSP/` 的**实际代码**。

**证据优先级**：原理图 / IO 接口表 ＞ Legacy 实际寄存器配置 ＞ 实测运行行为 ＞ Legacy 注释 ＞ 工程分析文档 ＞ 推测。不得只凭文档推测覆盖实物与源码。Legacy 文本可能使用 GBK 编码，搜索失败不能解释为源码不存在。

---

## 2. 已完成工作及禁止回退的裁决

### 2.1 BSP 已迁移模块

| 模块 | 参考提交 | 已知状态/约束 |
|---|---|---|
| `bsp_pwm` | `ac7f29b` | TIM1 三相互补 PWM；`BSP_PWM_Start()` 与 SPL 输出使能**不是严格一一等价**；初始 CCR=0；实际波形待示波器验证 |
| `bsp_adc` | `9e50b4c` | ADC1/2/3；Legacy 六次读取 `W→V→BUS→W→V→BUS`，前 3 次丢弃；`HAL_MAX_DELAY`；无 DMA/Injected/新滤波 |
| `bsp_encoder` | `2290461` | TIM3；`HAL_TIM_Encoder_Start()` 配合单独 Update 中断，**不能**直接改成 `HAL_TIM_Encoder_Start_IT()` 开 CC1/CC2 中断 |
| `bsp_timer` | `12b9d9e` | TIM2/TIM4 时基；`HAL_TIM_PeriodElapsedCallback` **只在 `BSP/bsp_timer.c` 定义**，TIM3 回调分支必须保留 |
| `bsp_motor_enable` | 尚待实施 | 本轮主要目标，PF10 默认禁能 |
| `bsp_uart` | 尚待实施 | 本轮不抢先迁移；下阶段 USART1 PB6/PB7, 115200 8N1 |

原交接记载：`bsp_timer` 完成时 FLASH 约 17620 B；**后续产物大小必须重新实测**，不可复述为最新构建数字。

### 2.2 已裁决技术事项

- `Template222_A` 为**功能行为基线**；`Template222-100` 为历史对照。两套 Legacy 只读、不可覆盖。关键不同：`enable_flag 0→1`，`Speed_Kp 0.02→0.08`，`Speed_Ki 0.0001→0.0015`；严禁混用或暗中重新调 PI。
- TIM2/TIM4 启动前**维持不清 UIF**，尊重 Legacy 不对称，沿用 `fc320f7` 既定裁决。不得把外部分支的防御性清 UIF 补丁重新混入。
- R-01：编码器 `CNT + overflow*65536` 在硬件计数器回绕与软件 ISR 更新时间不一致时可能出现 ±65536 跳变；同抢占优先级不能消除此风险。保持 `NEEDS MEASURE`，未来独立修复。
- TIM3 只开 Update 中断，不额外启用 CC1IE/CC2IE。
- 已有启动顺序：`BSP_ADC_Start → BSP_Timer_Start → BSP_Encoder_Start → BSP_PWM_Start`。本次安全禁能可以在此前增加**重复的 ForceDisable**，但不得改变 ADC / TIM2 / TIM4 / Encoder / PWM 相互顺序。
- TIM2 的 `theta/Speed_actual` 更新、TIM4 的 XY 插补、ADC 换算层、FOC 数学和 ISR 消费者尚未完整迁入。不得因为计时器诊断 Tick 能增长，就宣称运动控制复现完成。
- `BSP_ADC_ReadAll()` 尚无消费者；按现有 D-11 约束，后续与 Legacy 对齐在**主循环**调用，禁止简单塞入 FOC ISR。
- 既有差异编号 `D-01` 至 `D-18` 已占用；新增从**实际仓库中的下一个空号**开始。原交接建议下一空号为 `D-19`，先核对，不得覆盖旧条目。

### 2.3 五级验收模型

| 等级 | 判据 | 当前不可逾越的边界 |
|---|---|---|
| L1 | 源码与行为映射、参数、方向、调用顺序对照 | 功能差异必须如实登记 |
| L2 | CMake/GNU Arm 编译链接通过，ELF/MAP/BIN/HEX、符号证据 | 不等于时序或电气正确 |
| L3 | 主机对照测试与边界测试 | 未系统完成 |
| L4 | **无功率**板级波形、时钟、GPIO、ADC、Encoder 和 IRQ 实测 | 没有示波器/板卡证据不能标 PASS |
| L5 | 保护链闭环后带功率控制验收 | **BLOCKED** |

---

## 3. 第一项执行：PLLQ 从 4 恢复为 7（单独提交）

### 3.1 背景与证据

在交接后的源代码核查记录中发现：

```text
Legacy/Template222_A/USER/system_stm32f4xx.c:
    PLL_Q = 7  （与 336 MHz VCO 配合，48 MHz 域 = 336/7 = 48 MHz）
权威配置规范：PLLQ = 7
Core/Src/main.c SystemClock_Config():
    RCC_OscInitStruct.PLL.PLLQ = 4;
DMF407_FOC_HAL.ioc:
    RCC.PLLQCLKFreq_Value=84000000
    RCC.48MHZClocksFreq_Value=84000000
```

**判定**：当前 `.ioc` 与生成代码 Q=4、输出约 84 MHz，与 Legacy 和配置规范 Q=7 不一致。SYSCLK=168 MHz 所依赖的是 PLLP，因而现有 TIM/FOC 主时基不因这个 Q 差异而直接改变；但 USB/SDIO/RNG 所需 48 MHz 域不等价。即使目前未使用这些外设，仍应关闭此项 L1 配置偏差。执行前请再次核对**实际生效**的 Legacy 预处理分支与 VCO 参数，避免套用注释中的非生效宏。

### 3.2 具体操作

1. 先保存 `git status`、`.ioc` 和 `Core/Src/main.c` 的修改前证据。
2. 使用 **STM32CubeMX GUI 的 Clock Configuration** 将主 PLL 的 Q 设为 **7**，目标 PLLQ 域为 **48 MHz**；保存 `.ioc` 后 **Generate Code**。若可经验证的 CubeMX CLI 原生生成流程可用，也必须与 GUI 配置语义一致。
3. **禁止只改 `main.c` 中 `PLLQ = 4` 为 `7`**，也禁止仅替换 `.ioc` 的派生频率字符串而不重新生成。不得手写绕过 CubeMX 的时钟树。
4. 审核 `.ioc` 和生成代码的真实 Diff：预期核心变化为 PLLQ=7 / 48 MHz 相关配置；不得意外改变 GPIO、ADC、TIM1/2/3/4、USART、NVIC、时基、时序和用户区代码。
5. 再次核实：`HSE=8 MHz`、`PLLM=8`、`PLLN=336`、`PLLP=2`、`SYSCLK=168 MHz`、`APB1 TIM=84 MHz`、`APB2 TIM=168 MHz`。不得为了设置 Q 擅自改 PWM PSC/ARR 或时钟分频。
6. 使用本机 GNU Arm CMake 全量构建，并验证 ELF/MAP/BIN/HEX 与核心符号。若 CubeMX 再生成影响文件过多，**先审计所有新增变化**，不得简单覆盖已迁移 BSP / 用户代码。
7. 更新 `V1_Legacy_Mapping.md`、工作日志中此项的证据与状态。建议在下一可用编号（如 `D-19`）登记“PLLQ 配置差异发现并校正”，写明旧值、新值、时钟域影响和证据。
8. **PLLQ 独立提交**：仅在生成结果和构建通过后，用明确的 `fix(clock): ...` 主题提交；提交记录附生成代码的读回证据。

**此任务验收**：L1 配置核对闭环、L2 GNU Arm 构建通过。若无板级时钟测量，则 L4 继续 `NEEDS MEASURE`，不能写 PASS。

---

## 4. 第二项执行：PF10 默认禁能与 bsp_motor_enable（单独提交）

### 4.1 已作出的安全裁决

原交接 §5.1 的 A/B 选项已确定选择 **B（安全优先）**。不再要求 Codex 发起相同选择题。

```text
引脚：PF10
网络：PM1_CTRL_SD
原始 Legacy：time1.c 中 GPIO_SetBits(GPIOF, GPIO_Pin_10)；默认高
当前 V1 生成代码：
    HAL_GPIO_WritePin(PM1_CTRL_SD_GPIO_Port,
                      PM1_CTRL_SD_Pin, GPIO_PIN_SET);
本次目标：CubeMX GPIO Output Level = Low，生成代码 GPIO_PIN_RESET
```

根据已有 `ATK-PD6010B_V1.0原理图.pdf` 与硬件安全审计记录，**板级** `CTRL_SD=0` 为禁止 H 桥输出、`CTRL_SD=1` 为允许输出；不要拿 IR2110S 芯片内部 `SD_IN` 的电平含义反推整个板级控制脚。**实际驱动板版本和 R70 装配尚需实物核对**，因此把 `PF10=Low` 称为“依据当前图纸的目标禁止态”，而非已经通过实测证明的最终安全状态。

V1 功能基线中的 `enable_flag=1`、PI/FOC 参数保持在冻结的 Legacy 文件里，不允许用其驱动实际使能输出。当前应用层必须具备**绝无自动进入使能态的静态保证**。

### 4.2 CubeMX 初始化必须先改变

1. 核对 PF10 在 `.ioc` 中的输出电平、Pull、Speed、Mode 和生成 `gpio.c` 的实际语句。
2. 在 **CubeMX GPIO 配置**中将 `PF10 / PM1_CTRL_SD` 的 Initial Output Level 调整为 **Low**，其他 Legacy 引脚特性在此次不要顺手优化：Output Push-Pull、Pull-down、既有 Speed。保存 `.ioc` 并 Generate Code。
3. 确认输出锁存位（ODR）的禁止值尽可能在 PF10 切换到输出模式**之前**预置，检查生成代码中 `HAL_GPIO_WritePin(...,GPIO_PIN_RESET)` 与 `HAL_GPIO_Init()` 的顺序。任何上电复位到 GPIO 初始化之间的窗口不能由此宣称已解决。
4. **不允许靠在 `main()` 稍后补一个 `HAL_GPIO_WritePin(RESET)` 来“掩盖”** `.ioc` 的默认 High。后补禁能可作第二道措施，但原始输出初始化必须是 Low。
5. 检查 CubeMX 再生成是否回退了 PLLQ=7、TIM1/2/3/4 回调、ADC/Encoder/BSP、NVIC、PWM Pulse=0、HAL 用户区域和源文件列表。

### 4.3 BSP 模块最小实现范围

新增：

```text
BSP/bsp_motor_enable.h
BSP/bsp_motor_enable.c
```

职责严格限定：

- 对外提供明确命名的 **`BSP_MotorEnable_ForceDisable()`** 或等价接口，唯一负责写入 PF10 禁止状态。
- 如提供状态读取接口，只能称作**“软件命令状态 / GPIO 锁存值 / 引脚观测值”**；不可以此认定驱动板确实禁能。不要把 GPIO 命令状态伪装为 OC_OUT/R70/硬件门极真实反馈。
- **本阶段不向应用层开放可触发 PF10 High 的正常使能接口**。不得调用或暗中实现上电自动使能、延时自动使能、重试后使能、调试默认使能、故障清除后自动使能。
- 不要建立会错误允许使能的空壳状态机；将未来 `SYS_INIT/READY/RUNNING/FAULT/ESTOP` 与硬件保护链作为 V2 需求登记即可。
- 模块封装 HAL GPIO 写操作；`main.c` 应仅通过 BSP（位于 CubeMX USER CODE）显式重复调用 ForceDisable，其他业务模块禁止直接写 PF10。
- 不碰 `BSP_PWM_Start()` 原有实现、不改 TIM1 MOE/CCR/BDTR/Break 等，**但要保留“PWM 输出可能已经使能，而板级驱动仍必须禁能”的明显风险提示**。
- 将 `.c` 加入现有 CMake target；头文件路径遵循已有 BSP 规范；杜绝空实现、未引用模块及重复 HAL 回调定义。

### 4.4 上电路径 / 静态代码审计

在实际工程全局搜索并核对：

```text
PM1_CTRL_SD
GPIOF / GPIO_PIN_10
HAL_GPIO_WritePin
HAL_GPIO_TogglePin
__HAL_GPIO_* / GPIOF->BSRR / GPIOF->ODR
BSP_MotorEnable_* / enable_flag
```

其中 `GPIOF / PIN10` 的其它用途和位操作须逐项区分。检查启动路径、定时中断、故障恢复路径与调试代码，防止隐蔽的重新置高。输出**调用点清单（文件 + 行号 + 是否可达 + 安全性）**。不可只 grep 两个字符串就断言所有路径安全。

检查 `main.c` 的 USER CODE 2 调用顺序：如新增 ForceDisable，建议在所有 `BSP_ADC_Start / BSP_Timer_Start / BSP_Encoder_Start / BSP_PWM_Start` **之前**调用；原有四者相互顺序不变。**实际硬件的最早初始化由 `MX_GPIO_Init()` 内预置 Low 保证**，不能只靠后续的 USER CODE 调用。

### 4.5 不能声称已解决的硬件风险

- 复位、烧录、Bootloader、MCU 高阻、HardFault、调试器暂停、掉电/上电时序：PF10 可能尚未由固件驱动；需要板级外部下拉和供电时序实测。
- 确认 PD6010B 实板和图纸的 CTRL_SD 允许/禁止极性、上拉/下拉、隔离与光耦链路。
- **R70** 是否焊接以及它的过流锁存/非锁存逻辑；`OC_OUT` 故障如何传播、如何清除、是否可能自动重新输出。
- PB12/BKIN 当前 V1 保持 GPIO Input，并不构成经过验证的 TIM1 硬件 Break 保护链；不得把 CubeMX 中 Break Enable 选项误写为“保护已接通”。
- 无保护链验证前，**禁止 DC 功率母线与电机驱动功率测试**。即使无功率 GPIO 测试，也必须保证功率级物理断开。

### 4.6 差异登记与单独提交

- 更新 `V1_Legacy_Mapping.md`：以剩余空编号（预期在 PLLQ 后如 `D-20`，须重新核实）登记本次安全偏离：Legacy PF10 High → 可下载 HAL 固件 PF10 Low；说明缘由、初始化阶段变化、受影响函数与保留未决项。
- 关联已有风险 `R-03` 与 `V2-SF-02`；说明**默认禁能措施已移入 V1 实施，但硬件安全闭环仍属 V2 未完成**，不可整体关闭 R-03/SF-02。
- 调整交接文档原 §5.1 的“待用户裁决”文字：写明本轮选择 B 且已实施/未实施的真实状态。不能覆盖原始交接记录；优先在新工作日志/更新版交接另存记录。
- 生成有效 GNU Arm 编译日志、相关符号和构建产物；保存输出初始化实际代码读回证据。
- 使用单独 `feat(safety): ...` 或 `fix(safety): ...` 提交，不混入 UART/PI/FOC/运动控制功能。

**此任务验收**：能从 `.ioc`、`gpio.c`、BSP 和 `main.c` 静态确认命令默认禁能，构建通过；若没有**实板无功率**电平测试，L4 仍保持待验证；L5 继续 BLOCKED。

---

## 5. 建议操作顺序与分阶段停机点

**阶段 A：证据恢复（只读）**

- [ ] `git log`、`git status`、`cmake --list-presets` 与现有构建基线记录。
- [ ] 读取双 Legacy、配置规范、原理图与 V1 差异表，确认 D-19 后实际空位。
- [ ] 用真实文件行号确认 PLLQ=4、PF10=High 和全部其他 PF10 可达写入路径。

**阶段 B：修正 PLLQ（独立 Commit 1）**

- [ ] CubeMX 中 Q=7、48 MHz，保存 `.ioc` 与 Generate Code。
- [ ] 检查 RCC、TIM 时钟、所有生成文件 Diff 和用户代码保留情况。
- [ ] GNU Arm 构建、产物证据、登记新差异项。
- [ ] 仅提交本阶段实际变更；核对本地 HEAD。

**阶段 C：PF10 默认禁能与 BSP（独立 Commit 2）**

- [ ] CubeMX 将 PF10 Initial Low，Generate Code。
- [ ] 新增 `bsp_motor_enable` 并接入 CMake、USER CODE；无使能 API 可达。
- [ ] 对全部写入路径和复位窗口风险进行审计。
- [ ] GNU Arm 构建，提交独立安全偏离项与证据。

**阶段 D：审核、同步、报告**

- [ ] `git diff --check`，完整源码读回，核对未改动 FOC、PI、TIM2/TIM4 UIF 与原 Legacy。
- [ ] 文档标注 L1/L2 实证与 L3/L4/L5 缺项。
- [ ] 本地按提交序列检查，每个 commit 仅含对应阶段内容。
- [ ] 代理推送 GitHub，核对 `origin/main` 对应最新 HEAD；失败要报告真实状态，不可声称成功。
- [ ] 汇报全部变更文件、行号、测试结果、生成代码与风险。

**阻断规则**：若 CubeMX 不可运行、工程缺少 Firmware Package、生成变化超出可审核范围、目标 GPIO 极性与新硬件证据冲突、或链接失败，保留已验证成果，清晰列出阻断点；**绝不为了报 PASS 手写篡改生成代码或降低电气安全边界**。不问已经由用户回答的问题。

---

## 6. 编译、审计与 Git 操作模板（按本机实际情况调整）

```powershell
Set-Location 'C:\Users\xuhengyi\Desktop\item\课设\DMF407_FOC_HAL'

# 每阶段开始/结束
 git status -sb
 git diff --check
 git diff --stat
 git diff -- DMF407_FOC_HAL.ioc Core/Src/main.c Core/Src/gpio.c CMakeLists.txt

# 以实际 preset 或已有 Debug 路径执行完整构建
 cmake --list-presets
 cmake --build build/Debug

# 记录实际产物和日期，不要复制旧日志数字
 Get-ChildItem -Recurse -Path build/Debug -Include *.elf,*.map,*.bin,*.hex |
     Select-Object FullName,Length,LastWriteTime
```

Git 写入要求：

1. 执行前确认作者身份可用，若无全局身份，按交接文档使用**仅本次命令**的配置：

```powershell
git -c user.name='xu2005' -c user.email='140651546+xu2005@users.noreply.github.com' commit -F '<UTF-8提交消息文件>'
```

2. **点名 `git add` 文件**，不能随手 `git add .` 导入临时补丁或原本忽略的 Drivers/build。
3. 仅当本机 SOCKS 服务实际监听 `127.0.0.1:10808` 且用户现有代理可用时，按原交接经验使用单次代理参数：

```powershell
git -c http.proxy=socks5h://127.0.0.1:10808 `
    -c https.proxy=socks5h://127.0.0.1:10808 push origin main
```

4. 代理不可用或推送失败时，只记录本地 commit 与远端未同步状态；不修改全局 Git 代理、不泄露凭据、不承诺已经上传。
5. 新克隆仅有 Git 跟踪文件时，`Drivers/` 可能缺失；先用合法安装的 STM32Cube FW_F4 对应版本及 CubeMX 生成补齐。不可将缺 `Drivers/` 误判为 BSP 源码失败。
6. `Legacy/** -text` 的只读性质应保留；不修改或重新编码 Legacy 文件，不覆盖 `.gitattributes`。

---

## 7. 原交接仍然开放的事项：本轮不得宣称通过

| 项 | 保留状态 / 后续动作 |
|---|---|
| TIM2 约 1 ms、TIM4 约 1.310 ms | `NEEDS MEASURE`；尤其首次 UIF 触发时刻 |
| TIM2/TIM4 启动 UIF | 已裁决**不清**；首中断实际行为 `NEEDS SCOPE CONFIRM` |
| TIM3 Encoder R-01、方向、每转计数、溢出 | `NEEDS TARGETED BOUNDARY TEST` / L4 待验证 |
| TIM1 PWM 主/互补、CCR=0 初态与死区 | `NEEDS SCOPE CONFIRM` |
| ADC 与 PWM 相位、采样阻塞和换算校准 | `NEEDS MEASURE`；保留旧 6 次读取顺序 |
| 速度角度 / XY 插补 / FOC ISR 调度 | 尚未按 Legacy 完整接回 |
| PF10 与 PD6010B 的真实禁止态、R70、BKIN | 独立电气验证项；软件默认低不等于硬件 PASS |
| L3 全链回归 / L4 板上无功率 / L5 带功率 | 未完成 / 未验收 / **BLOCKED** |

本轮之后的既定顺序为：**`bsp_uart` → Encoder/ADC/PWM 三链独立回归 → Legacy 数学层 → 调度接回 → 无功率板级验证 → 经另行授权及保护链验收才考虑功率级**。

---

## 8. Codex 最终回报格式（必须有实证）

请以**真实文件和命令输出**填写，不要直接抄录旧交接数字：

```text
[项目] DMF407_FOC_HAL
[执行者] Codex 6.1 Sol / Windows 本机
[执行前 HEAD] <hash>
[执行后 HEAD] <hash>
[origin/main] <hash 或无法核验原因>
[工作区] <git status -sb 原文>

[Commit 1 — PLLQ]
- 提交：<hash>
- .ioc 设置与生成 main.c 中实际 Q 值：<源文件:行号>
- SYSCLK/APB时钟不变证据：<文件/日志>
- 修改文件：<列表>
- 差异编号：<实际登记编号>
- GNU Arm / ELF / MAP / BIN / HEX：<真实结果>

[Commit 2 — PF10 / bsp_motor_enable]
- 提交：<hash>
- PF10 .ioc 默认低与 GPIO 初始化顺序：<源文件:行号>
- BSP 提供的 API、消费者和 CMake 集成：<源文件:行号>
- PF10 全部可达写入点：<列表>
- 差异编号、R-03/SF-02 关联：<证据>
- GNU Arm / ELF / MAP / BIN / HEX：<真实结果>

[L1] <完成 / 未完成及证据>
[L2] <完成 / 未完成及证据>
[L3] <未系统开展，除非有真实测试>
[L4] <NEEDS，无功率硬件实测情况>
[L5] BLOCKED
[POWER-STAGE ENABLE] BLOCKED
[遗留风险] <逐项列出>
[GitHub 推送] <成功 / 失败及证据>
[下一步] bsp_uart
```

### 最终禁令

**禁止把编译通过、GPIO_PIN_RESET 已生成、GitHub 推送成功三者中的任何一项等同于电机硬件安全通过。禁止自动使能 PF10、连接功率母线、启动双轴。禁止修改或覆盖 Legacy、现有 FOC 算法与 PI 参数来“提高完成度”。**

---

*本任务书吸收原《交接_2026-10-10_bsp_timer完成.md》所有关键阶段状态和未结事项，并用用户最新裁决替换旧交接 §5.1 的待选 A/B 状态，同时补入交接后发现的 PLLQ=4/7 配置差异。执行成功与否须以 Codex 本机证据另行判定。*
