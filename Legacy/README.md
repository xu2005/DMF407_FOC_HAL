# Legacy 归档说明（Phase 0 · Frozen）

> 状态：**FROZEN / 只读**
> 归档日期：2026-10-10
> 用途：V1 Legacy 等价迁移的**行为基线**。任何冲突按《引脚时钟配置规范》§1.1 的证据优先级裁决，不得用"更现代/更规范"覆盖而不登记。

---

## ✅ 已裁决：V1 唯一行为基线 = `Template222_A`（2026-10-10 拍板）

> **V1 唯一行为基线确定为 `Template222_A/`。`Template222-100/` 仅作为历史参考版本保留，不参与 V1 参数裁决。**
>
> 使用 A 版 `enable_flag=1`、`Speed_Kp=0.08f`、`Speed_Ki=0.0015f`。后续所有 SPL→HAL 对照、FOC 参数、启动行为均以 A 版为准，不再因 A/B 差异停下。

背景说明：任务书 §1.1 / §3 字面上写的路径是 `Legacy/Template222-100/`，而磁盘上名叫 `Template222-100` 的那份是**较早**的快照（主要文件 2025-11-19）。A 版（`Template222_A/`，主要文件 2025-12-03）与本会话早先的项目决策一致（"以 A 版为准"），且其参数是后来调试的产物。用户已明确拍板：以 A 版为准，任务书的字面路径理解为笔误，实质基线是 A 版。

---

## 目录对照

| 本目录 | 磁盘原始路径 | 时间线 | 备注 |
|---|---|---|---|
| `Template222-100/` | `课设\Template222-100(1)\Template222-100\` | 主要文件 2025-11-19 | 任务书字面所指 |
| `Template222_A/` | `课设\综合项目实训\综合项目实训\实验 Template224\实验 Template222\` | 主要文件 2025-12-03 | 你此前拍板的 A 版 |

两者是同一个 Keil 工程（`Template.uvprojx`）的两份快照，目录结构一致：`CORE/ FWLIB/ SYSTEM/ usart/ USER/`。

---

## 两版的完整实质差异

对 `USER/` 逐文件 diff，**改动共 9 个文件，其中只有 2 个有实质行为差异**：

### 1. `USER/globals.c` —— 3 个值不同 ★ 影响行为

```c
// Template222-100 (B)              // Template222_A (A)
int   enable_flag = 0;              int   enable_flag = 1;   // 上电即使能，便于启动验证
float Speed_Kp    = 0.02f;          float Speed_Kp    = 0.08f;  // 适度提高启动响应
float Speed_Ki    = 0.0001f;        float Speed_Ki    = 0.0015f; // 适度提高积分以克服静摩擦
```

其余 `id_Kp/id_Ki/iq_Kp/iq_Ki`、`Speed_Ref=100.0f`、限幅值等**两版完全相同**。

- `enable_flag`：B 上电不使能（更保守）；A 上电即进入闭环
- `Speed_Kp / Speed_Ki`：A 明显调高了启动响应与积分

### 2. `USER/time2.h` —— 少一行声明

```c
+ void TIM4_IRQHandler(void);   // 仅 A 版有
```

### 3. 其余 6 个文件：仅注释与 IDE 状态

`main.c` / `test.c` / `time2.c` / `globals.h`：**只有注释差异，无代码差异**。
`Template.uvprojx` / `Template.uvoptx` / `time5.c`：工程文件状态与空文件，无行为影响。

---

## 对 V1 的影响

| 项 | 若基线取 B | 若基线取 A |
|---|---|---|
| `Speed_Kp` | 0.02 | **0.08** |
| `Speed_Ki` | 0.0001 | **0.0015** |
| `enable_flag` 初值 | 0 | **1** |

任务书 §9 明确禁止"修改 PI 参数"。**在基线没定之前，PI 参数就是悬空的** —— 这必须由你裁决，不能由我推断。

> 另注：无论取哪一版，`enable_flag` 都与 V2 的"PF10 安全默认态"议题直接相关。A 版的 `enable_flag=1` 意味着上电即闭环，配合 Legacy 的 PF10 上电置高，是**无自检、无状态机的直接上电使能**。V1 按 Legacy 复刻，V2 才是整改点。

---

## 归档完整性

| 项 | Template222-100 | Template222_A |
|---|---|---|
| 文件数 | 118 | 119 |
| 大小 | 2 948 KB | 2 955 KB |
| 哈希清单 | `SHA256清单.txt` | `SHA256清单.txt` |

**已收录**：`CORE/`（CMSIS 内核 + 启动文件）、`FWLIB/`（STM32F4 SPL 全部 inc/src）、`SYSTEM/`、`usart/`、`USER/`（含 `Template.uvprojx` / `Template.uvoptx`）、`SHA256清单.txt`。

**已排除并说明理由**：

| 排除项 | 理由 |
|---|---|
| `OBJ/`（各 134 个文件） | Keil 编译输出，可由源码重建 |
| 工程根目录 169 个散落编译产物（`.o/.crf/.d/.axf/.htm/.hex/.map/.lnp/.sct/.dep/.browse/.lst`） | 同上 |
| `*.uvguix.*`（各 6 个） | Keil GUI 布局偏好，非工程内容 |
| `JLinkLog.txt` / `JLinkSettings.ini` / `EventRecorderStub.scvd` | 调试器本地日志与设置 |

被排除项在磁盘原路径仍完整保留，未做任何删除。

---

## 使用纪律

1. 本目录及其子目录**视为只读**，V1/V2 的任何迁移都不得原地修改。
2. 引用 Legacy 行为时必须给出 `文件:行号`，例如 `Template222_A/USER/main.c:57`。
3. 若发现归档内容与磁盘原始路径不一致，以磁盘原始路径为准，并更新本说明与哈希清单。
4. 验证时遵循任务书 §12：**磁盘实际文件 > Preview > 对话附件旧快照**。