# DMF407_FOC_HAL

STM32F407IGT6 单轴 Legacy → HAL/CMake 迁移工程。活跃参考为 `Legacy/Template222_A`，`Template222-100` 仅作历史对照。冻结审计基线：`20be14d6358c44a7f06afa82afe76b78fc879606`。

**只允许无功率调试：物理断开功率直流母线，PF10 保持 Low。L4 未完成、L5 / POWER-STAGE ENABLE = BLOCKED。** PF10 软件 Low 不能证明驱动板极性、复位高阻窗口、R70、BKIN 或保护链有效。本工程没有驱动使能 API。

## 依赖与构建

V2.2 Motion 已加入有界S曲线、非阻塞XY插补、100槽运动队列和可取消的纯逻辑无功率App模拟。Host Debug/Release各104/104，ARM Debug/Release × OFF/ON均编译通过。默认示例 `{4096,0} × 2` 输出8192个可读回软件X事件；周期仅为轨迹数据，无物理脉冲输出。详见 [Motion实现与验证](Docs/V2.2_Motion_实现与验证.md) 和 [最新续作检查点](Docs/V2.2_Motion_续作交接.md)。

当前为 **V2.1 软件安全整改**：TIM1 首次初始化锁定 AOE=0、LOCK=1；默认 OFF 固件只开 TIM1 CEN/UIE，MOE=0、六路 CCER=0，不调用 PWM/PWMN Start。Host Debug/Release 各86项通过（原70保留，其中3项 KNOWN_RISK；新增16项），ARM Debug/Release × OFF/ON 实际编译通过，DWT=ON 另有编译证据。真实电平、ADC首触发与实时性仍需L4。详见 [TIM1修正报告](Docs/V2.1_TIM1安全修正报告_2026-10-11.md) 和 [逐项测试/构建证据](Docs/V2.1_测试与构建证据_2026-10-11.md)。

- `.ioc`：STM32CubeMX 6.12.0；STM32Cube FW_F4 **V1.28.3**。
- 本轮工具：CMake 4.3.1、Ninja 1.13.2、GNU Tools for STM32 14.3.1；主机 GCC 8.1.0（MinGW）。CI 使用 Linux GCC，实际版本在日志中输出。
- `Drivers/` 不入库。新克隆需先安装固定版本的 CubeF4 固件包。可在 CubeMX 打开 `.ioc` 后 Generate Code；生成后必须检查 USER CODE、PF10 初始 Low、PLLQ=7、PB12 配置、启动顺序及 CMake 业务源表未被覆盖。优先仅复制固件包的 HAL Driver、CMSIS 到本地 `Drivers/`，避免覆盖应用源码。
- 执行 `cmake -DPROJECT_ROOT=. -P cmake/check_dependencies.cmake` 预检；缺依赖时给出具体路径。仓库不自动下载或选择其他版本。预检是完整性检查，版本来源须由固定包和记录的 SHA256 核对。
- 可用 `scripts/check_cube_package.ps1 -PackageRoot <本地 STM32Cube_FW_F4_V1.28.3 目录> -ManifestPath <CSV 路径>` 将本地依赖逐文件与固定固件包核对。该操作验证本地包一致性，不提供厂商签名认证。

```powershell
cmake --preset Debug
cmake --build --preset Debug
cmake -S tests/host -B build/host -G Ninja -DCMAKE_C_COMPILER=gcc
cmake --build build/host
ctest --test-dir build/host --output-on-failure
```

主机测试独立于 HAL/CMSIS。`KNOWN_RISK` 标签代表成功复现 V1 风险，不代表关闭问题；新增 V2 修复测试单独登记。MCU 编译、主机计算和实板实时性能分别验收。

默认 `DMF407_NO_POWER_CONTROL=OFF`：主循环有限等待采集 ADC，TIM2 采集 V2 编码器，TIM1 只做 Tick 诊断；不运行 PI/SVPWM、不动态写 CCR。可选 ON 版本把 TIM2 角速计算、TIM1 控制计算与经过验证的 CCR 写入接通，仍只供断开母线后的无功率验证。ON 版本尚未通过 L4，不作为可直接运行电机的固件。

```powershell
cmake -S . -B build/no-power-control -G Ninja -DCMAKE_BUILD_TYPE=Debug -DDMF407_NO_POWER_CONTROL=ON
cmake --build build/no-power-control
```

V2.1 软件故障锁存后先拉低 PF10，再关 TIM1 UIE、MOE、六路 CCER 与 CEN，随后尝试所有 HAL Stop。AOE 在初始化时锁定为0，运行时不尝试清锁定位；发现旧 AOE=1 或 LOCK≠1 时拒绝启动并锁存配置故障。故障期间拒绝受检重启与 CCR 更新，复位 MCU 才重建状态。ADC 使用单次 2 ms 诊断超时（六次等待，受 HAL tick 与抢占影响），不是实时电流采样预算；默认 OFF 的 CC1 首触发仍需 L4，失败超时，不启动 PWM 绕过。20 ms 原始码龄期、2 ms 编码器粗时间检查亦只是诊断阈值。相邻样本实际位移必须小于32768，CNT无法检测漏整圈。

`DMF407_DWT_DIAGNOSTICS=ON` 可测 TIM1 IRQ 的 HAL/Callback/控制 C 调用区间，默认 OFF；记录 last/max/count，无 ISR UART/printf/动态内存。异常入/出栈、统计开销、暂停/抢占等须另外计入 L4 WCET，不能以主机计数或离线数学耗时替代。`python scripts/v21_validate.py` 可重跑本机完整矩阵并归档命令、退出码、SHA与全部日志。

## 门禁与范围

| 门禁 | 证据 | 状态 |
|---|---|---|
| L1 | 源码映射、配置与差异登记 | 软件审查，仍有硬件未知项 |
| L2 | ARM ELF/MAP/BIN 与依赖记录 | 见本轮整改报告 |
| L3 | 模块和无功率集成主机回归 | 见原始 CTest 日志 |
| L4 | MCU 逻辑电源下的 PF10/PWM/ADC/Encoder/BKIN 实测 | 未完成 |
| L5 | 功率级低能量运行 | BLOCKED |

TIM1 仍为 PSC=83、ARR=99、中心对齐、RCR=0；10:1 速度环分频不能理解为已经证明 1 kHz。TIM2 计算保留标称 1 ms；必须实测实际间隔。ADC 保留连续转换、TIM1_CC1 首触发和六次 W/V/BUS 轮询顺序；不代表同步采样。SVPWM 保留 Ts=100、Udc=24，不以采集母线值替换其参数。

TIM4 的 XY 轨迹规划尚未恢复。双轴前，TIM2 的 1 ms 时基与 PA15/PB3 上 PM2 编码器存在资源冲突，必须先重新分配时基。

实板验收与整改边界见 [整改报告](Docs/整改报告_2026-10-11.md)、[L4 验收表](Docs/L4_无功率验收_2026-10-11.md)、[V2 安全清单](Docs/V2_硬件安全修正清单.md)。
