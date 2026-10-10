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
