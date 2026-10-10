# DMF407_FOC_HAL · Codex 6.1 Sol 精简续作提示词

> 执行环境：Trae 中的 Codex 6.1 Sol；日期：2026-10-10  
> 目标：**低 Token、少工具调用、一次完成 `bsp_uart` 代码迁移并提交**。这不是新一轮全工程审计。

## 1. 当前基线

- 项目：`C:\Users\xuhengyi\Desktop\item\课设\DMF407_FOC_HAL`，分支 `main`；以磁盘上的实际 HEAD 为准，预期已包含 `6d01c13`（PLLQ=7）和 `4a43049`（PF10 默认 Low、`BSP_MotorEnable_ForceDisable()`）。
- 已完成：`bsp_pwm`、`bsp_adc`、`bsp_encoder`、`bsp_timer`、`bsp_motor_enable`（L1/L2）。本轮只做最后一个基础模块 **`bsp_uart`**。
- 冻结：`Legacy/`、FOC/PI 参数、TIM/ADC/PWM 配置、PF10 安全默认值。不触碰功率级；`POWER-STAGE ENABLE = BLOCKED`。

## 2. 执行任务（顺序执行，不扩展范围）

1. **最小读取**：运行 `git status --short` 和 `git log -1 --oneline`；只查看 Legacy 中实际使用 USART1 的相关函数、当前 `Core/Src/usart.c`、`Core/Src/main.c`、`BSP/` 目录及 `CMakeLists.txt` 相关片段。不要重读全部文档，不搜索整个仓库，不遍历所有历史提交。
2. **函数映射**：用极简表格列出 Legacy UART 函数/调用点 → HAL/BSP 接口；以实际源码为准，不猜协议。不改变已确定的 USART1 `PB6=TX`、`PB7=RX`、`115200 8N1`。
3. **最小实现**：新增 `BSP/bsp_uart.h`、`BSP/bsp_uart.c`，使用现有 `huart1` 和 ST HAL。只封装真实会用到的最小发送/接收能力；保持现有 Legacy 数据流语义。不要自造协议、环形缓冲区、队列、DMA、RTOS、复杂状态机、重试框架或通用驱动层。若原代码并未实际使用接收功能，不要预先实现。
4. **最小接线**：仅修改必要的 `CMakeLists.txt` 与 `main.c` USER CODE 区域。不要修改 CubeMX 自动生成的普通代码块、`.ioc`、HAL 库或其他 BSP。若 USART1 初始化已由 `MX_USART1_UART_Init()` 完成，**不要重复初始化**。
5. **最低验收**：只运行一次增量 `cmake --build build/Debug`；失败时针对错误修复并重编。检查 `git diff --check` 与变更文件列表，确保无非预期修改。编译通过仅记为 L2，不宣称 UART 硬件通信已通过。
6. **记录与提交**：只在现有工作日志追加几行“做了什么、编译结果、尚未测什么”；如有真实 Legacy 差异，再使用下一未占用 D 编号登记，勿为纯封装增造差异。完成一次独立 Git 提交并推送；如网络直连失败，使用现有 `127.0.0.1:10808` SOCKS 代理单次重试（须确认端口在监听）。不要提交用户已有未跟踪文件。

## 3. Token 与工程简化纪律（严格遵守）

- **不做任何文件 SHA/MD5 哈希校验**；不逐文件验证 240 个 Legacy 文件，不生成重复证据包；仅以 Git Diff 确认本轮实际改动范围。
- **避免过度防御性编程**：不加入无依据的重复判空、多层错误封装、重试/回退框架、自诊断计数器、复杂日志系统，也不为“未来扩展”提前创建 API。仅保留 HAL 返回状态所必需的简洁处理，不制造无限阻塞或隐藏失败。
- **不做冗余工具调用**：同一文件不反复读取，已知配置不重新研究，构建成功不再 clean rebuild；不自动开展静态分析大扫除、格式化全仓库或批量迁移其他模块。
- **不扩大本轮目标**：不做 L3 全套回归、不迁移 FOC 数学、不接 TIM1 闭环、不开发 XY 双轴、不启动电机、不改驱动功率控制逻辑。
- **必要安全边界不可省略**：PF10 保持默认 Low，禁止引入任何自动使能调用，R-03/SF-02 和 L4/L5 仍不关闭。简化代码不等于移除硬件安全约束。
- **不写长篇过程叙述**：优先实际修改与编译；最终反馈不超过 12 行，列明文件、构建状态、commit、远端同步情况和剩余待测项目即可。

## 4. 交付格式

```text
bsp_uart：完成 / 阻断（明确原因）
HEAD：<commit>
修改文件：<简短清单>
构建：PASS/FAIL（0 error 如属实）
GitHub：已推送 / 未推送
L1/L2：<证据范围>；L3/L4：未验证
POWER-STAGE：BLOCKED
下一步：启动三条链路的针对性 L3 回归测试（另开任务，不在本轮执行）
```

**停止条件：** 当 `bsp_uart` 的必要代码、构建、最小文档与 Git 提交完成时立即结束，不继续自行扩展任务。
