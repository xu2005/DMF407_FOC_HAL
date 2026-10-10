# PI / SVPWM 独立迁移：L1 映射、L2 编译与 L3 定向回归

2026-10-10 北京时间 21:05 条件续作。会话记录确认前轮在 17:24 因
`usage_limit_exceeded` 中断；实际基线 `fe10ea6b136a682be188fa50de7e684a94ea1815`。
此前 `0d28f70` 已入库四个 Control 草稿，本轮沿用，不回退或重复提交草稿。

## 源码映射与参数

活跃算法来自 `Legacy/Template222_A/SYSTEM/ipark.c`，参数来自
`USER/globals.c`，调用顺序来自 `USER/test.c:71-104`。
`USER/foc_pi.c` 与被注释的 `SYSTEM/svpwm.c` 不作为算法依据。

| Legacy → 新接口 | 保留的计算与输出 | 状态 |
| --- | --- | --- |
| `foc_pi_speed` → `FOC_SpeedPIUpdate` | `sum_speed += Ki*err`；先积分限幅，再 `Kp*err+sum` 输出限幅；显式状态与返回 iq_ref | PASS |
| `foc_pi_current` → `FOC_CurrentPIUpdate` | d 后 q；各自 `sum += err` 限幅，再 `Kp*err+Ki*sum` 限幅；显式双积分状态、d/q 输出 | PASS |
| `foc_pi_position` → `FOC_PositionCalculate` | 全局 `Position_Ref_X` 的值改为入参（原形参 `Position_Ref` 被忽略）；`Kp*(ref-actual)+Kff*feed`，限幅 ±limit | 纯公式 PASS；全局副作用 DEFERRED |
| `SVPWM` → `FOC_SVPWMCalculate` | 原 bit 扇区、六组 t4/t6、过调制缩放、ta/tb/tc 排列；Ts=100、Udc=24 | 正常 1–6 PASS；零向量 KNOWN RISK |

V1 参数：速度 Kp=0.08、Ki=0.0015；电流 d Kp=0.5、Ki=0.05，q Kp=1、Ki=0.4；
积分与 PI 输出均限幅 [-18,18]。位置 Kp=10、Kff=5、limit=1000。
参数由调用者明确传入，状态由调用者持有，不自动复位，不增加抗饱和或滤波。
位置测试仅使用差值适配 int32_t、正 limit 的原有有界域。
`id_show/iq_show` 截断诊断、`Speed_Ref/iq_Ref` 全局写入、位置调度和实时闭环均 DEFERRED；
Legacy 位置调用本身在注释块中，不能宣称位置全链路等价。

## 接口显式化差异：零向量未修复

Legacy sector=0 的 default 不赋值 ta/tb/tc，原调用局部结构未初始化。
新接口返回 1 表示正常三相输出，返回 0 表示无有效三相输出；始终写 sector，
无效路径不赋值也不读取 ta/tb/tc。该返回状态是新增接口契约，**不是 Legacy 原有行为**。
输出结构省去仅供原内部使用的中间量，正常 ta/tb/tc 仍是 Legacy 比较时刻单位，
不是自行归一化的占空比（本轮非零向量值域 [0,50]）。不写全局 tcm、不写 TIM1/CCR。
零向量测试只断言返回状态和 sector=0，不执行 Legacy 零向量路径、不比较三相字段。
其 CTest 成功仅说明接口状态符合契约，**KNOWN RISK 仍开放**。

## 本轮验证（各执行一轮）

`tests/host/freeze_pi_svpwm_vectors.py` 只读提取原 clamp、PI、SVPWM 函数，
直接提取 globals 初始化及引用原 ipark.h，以现有 MinGW gcc、32 bit long、`-O0` 独立执行。
冻结 `pi_svpwm_vectors.h` 不读取新 Control 源码；正常测试无需运行生成器。
真实 `Control/foc_pi.c` / `foc_svpwm.c` 参与 host 编译，没有算法 Mock。
绝对容差 1e-5（50 附近约数个 binary32 ULP），未启用 fast-math。

| 定向 CTest | 覆盖 | 结果 |
| --- | --- | --- |
| pi_speed | 8 次连续更新，更新前后状态、正负积分/输出限幅、零参考保留积分 | PASS |
| pi_current | 6 次 d/q 连续更新，更新前后状态、双向限幅、非零参考 | PASS |
| pi_schedule | 20 个离线 tick，速度第 10/20 次先更新，电流每次；2:20 调用和逐帧状态/输出 | PASS；无实时调度验收 |
| pi_position | 4 个纯公式输入，含 ±limit | PASS；副作用 DEFERRED |
| svpwm_normal | 14 个非零输入：六正常扇区、六过调制缩放分支、两轴边界；sector 与 ta/tb/tc | PASS |
| svpwm_zero | 仅无有效输出状态 | KNOWN RISK（断言通过） |

命令：`cmake --build build/host --target pi_svpwm_tests`；
`ctest --test-dir build/host -R '^(pi_|svpwm_)' -V`：6/6 断言组通过，0 失败。
未重跑旧 BSP、角度或三变换测试。

`cmake --build build/Debug`：增量成功，实际编译 Motor/motor_angle_speed.c、
Control/foc_transforms.c、foc_pi.c、foc_svpwm.c，生成 ELF/MAP/BIN/HEX。
新源编译无告警；构建沿用既有 CMake enable_language 顺序与 ELF LOAD RWX 提示。
源表纳入编译，不强留未引用 section；最终 ELF 无 FOC 数学导出符号，符合 gc-sections。
**运行时接线：无**；main/ISR/ADC/PWM/启动顺序均未改。

本轮仅 CMake 源表、host 测试/参考与本文；Control 草稿保持原提交。
PF10 默认 Low、仅 ForceDisable；R-01/R-03/SF-02 未关闭，L4 未验收，
**L5 / POWER-STAGE ENABLE = BLOCKED**。这些主机和编译证据不构成硬件验收。
