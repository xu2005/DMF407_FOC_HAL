# V1 数学层最小迁移 / L3 定向回归（2026-10-10）

执行前实际 HEAD：`6871971e645a0f4f2c59cef54e59ff2fd0e9587e`，未回退任务书参考提交。
只新增 Motor/Control 纯数学、定向测试和本文；旧 BSP、Legacy、.ioc、生成代码、ISR/main、安全策略均保持。

## 实际范围与来源映射

| Legacy 功能 / 来源 | 本轮模块与状态 |
|---|---|
| `USER/angle.c:60-101` calculate_motor_parameters；`USER/time2.c:48-54` 调用与全局输出 | **A PASS**：`Motor/motor_angle_speed.[ch]`，输入独立 raw CNT 与 total_count，显式 previous_count 状态，返回机械角/电角度/RPM；没有接回 TIM2 或 theta/Speed_actual |
| `SYSTEM/ipark.c:13-17` clarkTransform | **B 纯计算 PASS**：FOC_Clarke，u/v 全局读取改为输入，保留 float 与 1.73205080757f |
| `SYSTEM/ipark.c:18-23` parkTransform | **B 纯计算 PASS**：FOC_Park，保留 q/d 顺序与正负号；q_current 全局诊断镜像未接线，I_q 显式返回，不称整个全局行为已接回 |
| `SYSTEM/ipark.c:95-99` IPARK | **B 纯计算 PASS**：FOC_InversePark，sin/cos 仍由调用者输入，未改方向约定 |
| `SYSTEM/ipark.c:35-92` 位置/速度/电流 PI | **DEFERRED**：共享 sum_speed/sum_d/sum_q、Speed_Ref/iq_Ref、增益和限幅全局；速度积分为Ki×err、电流积分为err后乘Ki，两者规则不同；test.c:71-94 的速度环每10次、电流环每次调度和诊断副作用未拆分。未迁移、未改参数、未跑PI测试 |
| `SYSTEM/ipark.c:115-317` SVPWM | **DEFERRED**：svpwm 结构体输出与全局tcm1/2/3直接连到test.c:102-104 PWM写入；零向量sector=0走default，不给ta/tb/tc确定值，而test.c:49的局部结构未初始化。不能为得到确定的零输入结果自创默认占空比；未修原行为、未跑SVPWM测试 |

Keil `USER/Template.uvprojx:604-606` 的活跃实现是 SYSTEM/ipark.c；USER/foc_pi.c、SYSTEM/svpwm.c 没有对应活跃函数体。
A保留4000计数/转、4极对、offset初始float值0.546637118、PI=3.1415926f、delta_time=0.001f（以代码为准，未采用10 ms注释）。
保留uint32 raw减int32 previous的32位转换、Count<-32768 / >32767调整、先更新previous再算角速度、double fmod后赋float，以及原表达式次序。ARM的32位long以int32_t明确表达，避免LP64主机变成64位差值；参考生成要求long=32 bit。

## 测试与独立预期

MinGW GCC8.1.0 / CMake / Ninja，编译真实 Motor/Control 新源码，不使用HAL mock。
角度预期冻结于 `tests/host/math_angle_vectors.h`：生成器只读提取未修改Legacy角度函数，替换硬件读数并观察原局部机械角；不读取被测Motor源码。原公式和参数由Legacy源码提供，参考单独编译后固定，正常测试不重新生成预期。
三种变换使用独立固定向量；例如Park输入alpha3/beta-2/sin0.6/cos0.8，预期q=-3.4/d=1.2；Inverse Park独立输入d1.2/q-3.4，同sin/cos预期alpha3/beta-2，未拿另一被测函数的输出当预期。
绝对容差：角度2e-6 rad、RPM1e-3、变换1e-6；不开fast-math。数据类型和运算次序仍以源码审阅为依据，不把容差当作改公式的授权。

本轮一次配置/数学目标构建、一轮 `ctest --test-dir build/host -R '^math_' -V`，未重跑旧Encoder/ADC/PWM组。
math_angle为fixture前置项，A失败时不执行后续变换/传播测试。实际3/3断言组通过：

- **PASS math_angle**：10个固定向量，含零位、正负差值、连续两次采样、双向16位边界与32767/32768半圈差值边界。400计数/1 ms原式得5999.99951 RPM，未简化成6000。
- **KNOWN RISK math_angle_r01**：两种±65536 total_count错误，正常/错误结果都与各自冻结参考核对；成功表示风险传播被重现，R-01未关闭。
- **PASS math_transforms**：Clarke3、Park4、Inverse Park4个固定向量，覆盖零、一般、正负和轴向。

```text
KNOWN RISK R-01 total error=-65536: mech 1.86611319->5.73654795, elec 1.18126774->4.09663677; raw-based rpm stays 15
KNOWN RISK R-01 total error=65536: mech 5.73497725->1.86454153, elec 4.09035397->1.17498112; raw-based rpm stays -15
100% tests passed, 0 tests failed out of 3
```

运行/参考生成命令见 [主机测试说明](../tests/host/README.md)。可执行 `build/host/math_host_tests.exe`，原始CTest输出 `build/host/Testing/Temporary/LastTest.log`（构建目录未提交）。
一次ARM `cmake --build build/Debug` exit0 / `ninja: no work to do.`；**新数学未加入固件源表、未由ARM目标编译或运行，未恢复闭环**。
`git diff --check`通过，Git Diff确认冻结文件无修改；未做SHA/MD5，未重新审计旧BSP。

**NEEDS HARDWARE / L4**：主机浮点/固定输入不能证明MCU math库、实际1 ms节拍、CNT读取交错、极性与校准、ADC/PWM/IRQ真实时序。PI/SVPWM与调度、全链数学尚未完成。
PF10 Low / ForceDisable 保持；**R-01、R-03、SF-02未关闭，L5 / POWER-STAGE ENABLE BLOCKED**。
