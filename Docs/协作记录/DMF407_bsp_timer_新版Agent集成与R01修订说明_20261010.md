# DMF407_FOC_HAL · bsp_timer 适配 6af4938 交接与 R-01 勘误

日期：2026-10-10。性质：**离线适配包**，不是最新 Windows 工作区已集成的凭据。

## 1. 源码范围与版本原则

- 用户上传工程 ZIP 内 Git HEAD：`b9731af`，上一次 ChatGPT 的 `bsp_timer` 集成补丁即以此为基础。
- 本次粘贴的 Agent 交接报告称最新 HEAD 为 `6af4938`（加入复现框架与 D-13 NVIC），**此最新提交未在可用源码 ZIP 内，无法独立核验**。
- 此前补丁改动 `Docs/V1_Legacy_Mapping.md`，其 **D-13 被用作 TIM2 差异**，与最新报告中的 **D-13 = NVIC 子优先级归一化**冲突。
- 本包把 `Docs` 变更排除在补丁之外，只对 `BSP/bsp_timer.c/h`、`CMakeLists.txt`、`Core/Src/main.c` 提供**代码补丁**，避免旧文档覆盖新框架。

## 2. 对 Agent 的实操命令（在最新 Windows 工作区执行）

先保存与检查当前工作，确保任何现存修改不丢失：

```powershell
cd 'C:\Users\xuhengyi\Desktop\item\课设\DMF407_FOC_HAL'
git status --short
git rev-parse --short HEAD
git log -3 --oneline
# 备份有实质改动的文件，绝对不能直接 git reset --hard
git apply --check '<实际存放路径>\DMF407_bsp_timer_仅代码补丁_兼容6af4938审核用.patch'
```

如果 `--check` 通过，应用补丁；如果失败，逐块人工合并，**不要**强制套补丁或回滚至 `b9731af`。保留新提交里的 `Docs/V1_复现验收框架.md` 与 `D-13`。

```powershell
git apply '<实际存放路径>\DMF407_bsp_timer_仅代码补丁_兼容6af4938审核用.patch'
cmake --list-presets
# 根据列表使用本工程真实存在的 preset
cmake --preset Debug
cmake --build --preset Debug
git diff --check
git diff --stat
```

工程 `Drivers/` 未进入 Git，须通过既有 CubeMX 固件包生成/恢复，**不能以缺 Drivers 的空仓库构建失败宣称补丁错误**。

## 3. 差异登记编号：从 D-14 开始

保留 Agent 已在 `6af4938` 提交的 **D-13 = NVIC**，把本轮差异在最新映射表中登记为：

- **D-14 · TIM2**：Legacy TIM2 Update ISR 调用 `calculate_motor_parameters()` 并写 `theta/Speed_actual`；现仅记诊断 Tick，**非行为等价**。
- **D-15 · TIM4**：Legacy ISR 清除 `motorX_pluse/motorY_pluse` 并累加位置信息；现仅记诊断 Tick，**非行为等价**。
- **D-16 · 启动 UIF 清除**：HAL 接口启动前显式清 UIF，与旧代码有意不同；需要板上首次中断时刻验证。

编号以最新仓库文档的实际空缺为准。如存在新的冲突，顺延编号并在提交中记录，禁止覆盖 D-13。

## 4. R-01 严谨勘误（非常重要）

粘贴记录声称：TIM2 和 TIM3 的优先级相同，因此 Legacy 中 `gtim_get_encode()` 读 CNT 和溢出数「天然互斥、不会竞争」。**该结论不成立。**

同抢占优先级仅能保证 **TIM3 ISR 不会打断 TIM2 ISR**，不能停止硬件 TIM3 计数器更新。

- 当扩展溢出计数为 `k`，TIM3 CNT 在 TIM2 ISR 读取之前恰好从 65535 回绕到 0；
- 此时 TIM3 的 UIF 可已置位，但 TIM3 ISR 被同级 TIM2 ISR 延后，软件溢出数尚为 `k`；
- `CNT + k×65536` 读得 `k×65536`，正确扩展数应为 `(k+1)×65536`，产生 **−65536** 的瞬态错误。

同理，**先读 CNT，后读溢出数**的两次采样也可能跨越回绕或被 ISR 打断。故 `R-01` 在原 Legacy TIM2 中断上下文也可能触发，不能限定为「只有从主循环调用才会出现」。

V1 行为迁移可以保留原公式，但必须标注 `NEEDS TARGETED BOUNDARY TEST`；不要以「相同抢占优先级」标 `PASS`。V2 应独立修复并验证边界情况：上溢/下溢、UIF 待处理、采样先后、转向反转、连续一致性、主循环/ISR 不同调用上下文。候选方案需要同时考虑 `CNT`、溢出数和 `UIF/DIR`，不可把简单禁中断或重复读取当作无条件正确的解法。

## 5. PF10 与功率级安全

已上传旧工程 `Core/Src/gpio.c` 可见 `HAL_GPIO_WritePin(PM1_CTRL_SD_GPIO_Port, PM1_CTRL_SD_Pin, GPIO_PIN_SET)`。此 GPIO 初始电平是**代码事实**；具体高低有效关系必须以电气原理图及驱动芯片使能链实测确认。

- `POWER-STAGE ENABLE BLOCKED` 保持不变，不得使用本补丁进行带母线电压电机试验。
- 不能因为 Legacy 的 `enable_flag=1` 或 PF10 初始化为高，就把默认带功率使能合理化。需要在单独安全修复提交中明确上电禁能、硬件拉电平、故障保护、急停、BKIN。

## 6. 可复核证据与验收边界

本包代码补丁已在用户上传的 `b9731af` 干净检出副本通过 `git apply --check`、`git apply`、`git diff --check` 及主机模拟测试。**没有**对最新 `6af4938` 工作区实测应用；**没有** GNU Arm ELF 链接、板上 TIM2/TIM4 周期测量、GitHub 推送。

本轮集成完成后暂仅为 TIM2/TIM4 HAL 定时器启动与诊断事件计数；`Control/Motor` 计算链和 `Motion` 消费链仍待迁移，不能将 L1/L2 写成 L4/L5 PASS。
