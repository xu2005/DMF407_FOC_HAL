/**
  ******************************************************************************
  * @file    bsp_timer.h
  * @brief   TIM2 / TIM4 定时器底层的 BSP 封装
  *
  * 对应 Legacy（基线 = Legacy/Template222_A/）：
  *   USER/time2.c — Time2_Init() / Time4_Init() /
  *                  TIM2_IRQHandler() / TIM4_IRQHandler()
  *
  * 职责边界（任务书 §4.3）：
  *   本模块只做定时器外设访问与 Update 中断统一分发。
  *   TIM1/TIM2 可通过启动前注册的 App hook 派发无功率诊断/计算。
  *   TIM4 XY 插补业务仍未迁入。真实周期与 WCET 必须独立实测。
  ******************************************************************************
  */
#ifndef __BSP_TIMER_H
#define __BSP_TIMER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* 时间基（固定，来自 Legacy main.c:56-57 实参 + .ioc + Core/Src/tim.c）
 *
 *   TIM2：PSC=83 / ARR=999 / Up / RCR=0 / CKD=DIV1 / ARPE=Disable
 *         84 MHz / 84 = 1 MHz → 1000 计数 = 标称 1.000 ms
 *
 *   TIM4：PSC=10 / ARR=10000 / Up / RCR=0 / CKD=DIV1 / ARPE=Disable
 *         84 MHz / 11 ≈ 7.636 MHz → 10001 计数 ≈ 标称 1.310 ms
 *         ★ ARR 不得改为 26715（见 V1_Legacy_Mapping.md D-05）
 */

/* V1 待实测项 —— 实测前禁止作结论
 *   (1) TIM2 中断周期实测（标称 1.000 ms）              = NEEDS MEASURE
 *   (2) TIM4 中断周期实测（标称 ≈ 1.310 ms）            = NEEDS MEASURE
 *   (3) 上电首次"伪"Update 中断是否实际发生（UIF 未清）= NEEDS SCOPE CONFIRM（D-16）
 */

/**
  * @brief  启动 TIM2 与 TIM4，均开启 Update 中断
  * @note   对应 Legacy：
  *           time2.c:21-32   TIM_SetCounter(TIM2,0) / TIM_ITConfig(Update) / TIM_Cmd(TIM2)
  *           time2.c:75-83   TIM_SetCounter(TIM4,0) / TIM_ITConfig(Update) / TIM_Cmd(TIM4)
  *         ★ 与 Legacy 一致：本函数**不清** UIF（见差异登记 D-16）。
  * @retval 无
  */
void BSP_Timer_Start(void);

/* TIM1 Update diagnostic, called after TIM1 counter startup.
 * Clear pending UIF, enable UIE; preserve timing, CCR and output configuration.
 * NVIC/DIER/UIF, RCR relationship and actual frequency require L4 measurement. */
int BSP_Timer_StartTim1Diagnostic(void);
uint32_t BSP_Timer_GetTim1TickCount(void);
int BSP_Timer_StartChecked(void);
void BSP_Timer_SetHooks(void (*tim1_hook)(void), void (*tim2_hook)(void), void (*encoder_hook)(void));

/**
  * @brief  读取 TIM2 Update 中断累计次数
  * @note   V1 新增诊断设施，Legacy **无**对应全局量（见差异登记 D-17）。
  *         仅用于无功率实测时确认中断在运行；
  *         **不得**据此宣称角度/转速链路已复现。
  * @retval 累计次数（上电后只增）
  */
uint32_t BSP_Timer_GetTim2TickCount(void);

/**
  * @brief  读取 TIM4 Update 中断累计次数
  * @note   V1 新增诊断设施，Legacy **无**对应全局量（见差异登记 D-17）。
  *         仅用于无功率实测时确认中断在运行；
  *         **不得**据此宣称 XY 插补链路已复现。
  * @retval 累计次数（上电后只增）
  */
uint32_t BSP_Timer_GetTim4TickCount(void);

#ifdef __cplusplus
}
#endif

#endif /* __BSP_TIMER_H */
