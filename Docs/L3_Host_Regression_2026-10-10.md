# L3 主机定向回归结果（2026-10-10）

执行前 HEAD：`f2926abab9ebcca7ed69ce01f653319428833d8b`。
环境：Windows / MinGW GCC 8.1.0（x86_64-w64-mingw32）/ CMake + Ninja + CTest。
只新增 `tests/host/` 与本文；直接编译真实 `BSP/bsp_encoder.c`、`bsp_adc.c`、`bsp_pwm.c`，未复制 BSP 业务逻辑，生产源码及 Legacy 均未修改。

| 测试组 | 实际结果 | 证据范围与 Legacy 对应 |
|---|---|---|
| Encoder 正常路径 | PASS | SET_COUNTER→Encoder_Start→CLEAR_UPDATE→ENABLE_UIE；只开 UIE，不开 CC1IE/CC2IE。正反转、65535→0 上溢、0→65535 下溢、负计数和反转。对应 encoderini.c:64-74、angle.c:23-37；保留已登记的 HAL 启动语句顺序 |
| Encoder R-01 | KNOWN RISK（可复现） | CNT 已回绕、软件 Update 处理仍未执行：上溢真实期望65536、实读0，误差-65536；下溢真实期望-1、实读65535，误差+65536。之后调用真实 OverflowTick，读数恢复；未修算法，风险未关闭 |
| ADC | PASS | ADC1→2→3 启动；六次严格 W→V→BUS→W→V→BUS，每次先 Poll(HAL_MAX_DELAY) 再 GetValue。前3值303/202/101丢弃，后3值W2048/V0/BUS4095返回。对应 SYSTEM/adc.c:62-64,79-82,92-99 |
| PWM | PASS | 六路 HAL start/stop 调用顺序；U/V/W→CCR1/2/3，0/一般值13,47,83/当前ARR99边界99,0,98；独立注入CCR7,11,19验证GetABC。对应 USER/test.c:102-104,111-113；不代表波形验证 |

运行命令见 [tests/host/README.md](../tests/host/README.md)。本轮配置/编译一次、CTest执行一轮，实际输出：

```text
PASS encoder: startup/UIE only, forward/reverse, upper/lower wrap, direction reversal
KNOWN RISK R-01 upper: CNT=0 ISR pending, expected=65536 observed=0 error=-65536
KNOWN RISK R-01 lower: CNT=65535 ISR pending, expected=-1 observed=65535 error=+65536
PASS adc: start 1/2/3, W/V/BUS/W/V/BUS, poll-before-read, discard first 3, retain raw last 3
PASS pwm: HAL start/stop call order, U/V/W to CCR1/2/3, zero/general/ARR boundary, independent readback
100% tests passed, 0 tests failed out of 4
```

CTest 的4/4表示三组正常断言及一组风险复现断言均满足，不表示 R-01 安全通过。
可执行文件：`build/host/bsp_host_tests.exe`；原始输出：`build/host/Testing/Temporary/LastTest.log`（本地构建目录，未提交）。
固件仅执行一次 `cmake --build build/Debug`，exit0 / `ninja: no work to do.`，未重跑全量构建。
`git diff --check` 通过；Git Diff 确认 BSP/Core/Legacy/CMake/.ioc 无修改，未进行文件哈希校验。

**NEEDS HARDWARE / L4**：Mock 仅模拟内存寄存器效果和 HAL 调用，EOC 视为已就绪，不模拟时间。真实编码器极性/每转计数/IRQ延迟与方向反转时序、ADC阻塞/采样相位/校准、PWM互补输出/死区/MOE/BKIN/电平均未验证；未覆盖真实 CNT/软件累计读取期间的并发交错与极端 int32 溢出。
这只是三个现有 BSP 的定向 L3 软件回归，不代表角度速度、FOC数学、控制调度或全链路L3已完成。
PF10 Low / ForceDisable 约束保持；R-01、R-03、SF-02 均未关闭；**L5 / POWER-STAGE ENABLE BLOCKED**。
