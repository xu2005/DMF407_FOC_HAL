# DMF407 L3 主机定向回归

在工程根目录用本机 MinGW GCC 执行：

```powershell
cmake -S tests/host -B build/host -G Ninja -DCMAKE_C_COMPILER=C:/mingw64/bin/gcc.exe
cmake --build build/host
ctest --test-dir build/host -V
```

CMake 编译真实 `BSP/bsp_encoder.c`、`bsp_adc.c`、`bsp_pwm.c`。
只替换 HAL 调用和寄存器访问边界；测试没有另一份 BSP 业务实现。
四个进程分别测试 Encoder 正常路径、R-01 已知风险、ADC、PWM。
R-01 的 CTest 成功只表示断言复现了 ±65536 误差，不代表算法通过边界安全验收。

Mock 是确定性内存/调用模型，不模拟外设时间；ADC 默认 EOC 已就绪，PWM 仅记录调用及 CCR。
真实 IRQ 时序、采样等待/相位、引脚方向、电平、互补输出、死区、MOE/BKIN 均需 L4。
结果见 `Docs/L3_Host_Regression_2026-10-10.md`；L5 / POWER-STAGE ENABLE BLOCKED。

## 本轮角度/速度与纯变换定向回归

复用上面的配置命令，只编译新数学目标并只执行新增测试：

```powershell
cmake --build build/host --target math_host_tests
ctest --test-dir build/host -R '^math_' -V
```

`math_angle` 是 fixture 前置验证，失败时后续变换和风险传播测试不执行。
角度固定预期来自未修改的 Legacy 函数：可选运行 `python tests/host/freeze_angle_vectors.py`
重建参考（需要现有 gcc，且 long=32 bit），正常测试直接使用冻结向量、不调用生成器。
生成器只提取原函数、替换硬件读数并观察原机械角局部值，不读取被测新模块。
变换用独立固定输入/预期值，未用被测函数互相构造预期。
结果与 DEFERRED 项见 `Docs/V1_Math_L3_Regression_2026-10-10.md`。

## PI / SVPWM 定向回归

```powershell
cmake --build build/host --target pi_svpwm_tests
ctest --test-dir build/host -R '^(pi_|svpwm_)' -V
```

该目标编译真实 `Control/foc_pi.c`、`foc_svpwm.c`，无需 HAL Mock。
六组分别核对速度、电流、离线 10:1 状态序列、位置纯公式、正常 SVPWM 和零向量状态。
冻结预期由 `freeze_pi_svpwm_vectors.py` 提取并独立执行活跃 Legacy 函数生成，
直接沿用 `USER/globals.c` 参数及原结构头文件，不读取新 Control 源码。
可选 `python tests/host/freeze_pi_svpwm_vectors.py` 重建参考（现有 gcc、32 bit long）。
正常测试无需 Python。绝对容差 `1e-5`，约为最大输出 50 附近数个 binary32 ULP；不启用 fast-math。
零向量不执行 Legacy、不读取新结果三相字段，CTest 成功只表示状态契约满足，风险仍开放。
结果、L1 映射及接口差异见 `Docs/PI_SVPWM_L3_Regression_2026-10-10.md`。
