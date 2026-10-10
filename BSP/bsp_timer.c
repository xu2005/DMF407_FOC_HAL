/**
  ******************************************************************************
  * @file    bsp_timer.c
  * @brief   TIM2 / TIM4 定时器底层的 BSP 实现 + Update 中断统一分发
  *
  * Legacy 出处（基线 = Legacy/Template222_A/USER/time2.c）：
  *   :21      TIM_SetCounter(TIM2, 0)
  *   :23      TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE)
  *   :26-30   NVIC TIM2 = 抢占 1 / 子 1
  *   :32      TIM_Cmd(TIM2, ENABLE)
  *   :75      TIM_SetCounter(TIM4, 0)
  *   :77      TIM_ITConfig(TIM4, TIM_IT_Update, ENABLE)
  *   :78-82   NVIC TIM4 = 抢占 1 / 子 1
  *   :83      TIM_Cmd(TIM4, ENABLE)
  *   :36-58   TIM2_IRQHandler() 的 Update 分支（角度/转速，本阶段不迁入）
  *   :90-98   TIM4_IRQHandler() 的 Update 分支（XY 插补，本阶段不迁入）
  *
  * V1 约束（任务书 §9）：不改 PSC/ARR、不改 NVIC、不改 ARPE；
  *                       ★ 不给 TIM2/TIM4 补 UIF 清除（见 D-16，
  *                         Legacy 只有 TIM1/TIM3 清了 UIF，此不对称必须保持）。
  ******************************************************************************
  */

#include "bsp_timer.h"
#include "bsp_encoder.h"
#include "tim.h"

#define BSP_TIMER_TIM2  (&htim2)
#define BSP_TIMER_TIM4  (&htim4)

/* V1 新增诊断设施（差异登记 D-17）：Legacy 无对应全局量。
 * 只在中断里做只增计数，不参与任何控制计算，
 * 用途仅限无功率实测时确认中断在运行。 */
static volatile uint32_t s_tim2_tick_count = 0;
static volatile uint32_t s_tim4_tick_count = 0;

void BSP_Timer_Start(void)
{
  /* ---------------- TIM2 ---------------- */

  /* Legacy time2.c:21  TIM_SetCounter(TIM2, 0)
   * 注：MX_TIM2_Init() 的 HAL_TIM_Base_Init → TIM_Base_SetConfig 已把 CNT 置 0；
   *     此处显式复刻 Legacy 语句，保持一一对应且零行为差异。 */
  __HAL_TIM_SET_COUNTER(BSP_TIMER_TIM2, 0);

  /* Legacy time2.c:23 + :32  TIM_ITConfig(Update, ENABLE) + TIM_Cmd(ENABLE)
   *
   * 已核实 HAL 源码 stm32f4xx_hal_tim.c:459-494：
   *   HAL_TIM_Base_Start_IT() 先 __HAL_TIM_ENABLE_IT(htim, TIM_IT_UPDATE)（行 476），
   *   再 __HAL_TIM_ENABLE(htim)（行 489）—— 即「先开中断、后启动计数」，
   *   与 Legacy 的语句顺序（:23 在前、:32 在后）一致。
   *
   * ★ 不在此处清 UIF —— Legacy time2.c 全文没有 TIM_ClearFlag（D-16）。 */
  HAL_TIM_Base_Start_IT(BSP_TIMER_TIM2);

  /* ---------------- TIM4 ---------------- */

  /* Legacy time2.c:75  TIM_SetCounter(TIM4, 0) */
  __HAL_TIM_SET_COUNTER(BSP_TIMER_TIM4, 0);

  /* Legacy time2.c:77 + :83；同上，★ 不清 UIF（D-16） */
  HAL_TIM_Base_Start_IT(BSP_TIMER_TIM4);
}

uint32_t BSP_Timer_GetTim2TickCount(void)
{
  return s_tim2_tick_count;
}

uint32_t BSP_Timer_GetTim4TickCount(void)
{
  return s_tim4_tick_count;
}

/**
  * @brief  TIM 更新中断统一分发（覆盖 HAL 弱定义）
  * @note   业务逻辑不写入 stm32f4xx_it.c / main.c（任务书 §8）。
  *         本函数在 USER CODE 4 的旧位置（main.c）迁入本模块。
  * @retval 无
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  if (htim->Instance == TIM2)
  {
    s_tim2_tick_count++;

    /* Legacy time2.c:48  TIM2_IRQHandler 内：
     *   calculate_motor_parameters(&local_theta, &motor_speed);
     *   theta = local_theta;  Speed_actual = motor_speed;
     *
     * ★ 本阶段（定时器底层迁移）**不迁入**：依赖 angle.c 的角度/转速换算，
     *   归 Motor/Control 层后续阶段。当前 TIM2 分支只有诊断计数，
     *   **不代表角度/转速链路已复现**。 */
  }
  else if (htim->Instance == TIM3)
  {
    /* Legacy angle.c:15-32  TIM3_IRQHandler 的 Update 分支 */
    BSP_Encoder_OverflowTick();
  }
  else if (htim->Instance == TIM4)
  {
    s_tim4_tick_count++;

    /* Legacy time2.c:90-98  TIM4_IRQHandler：
     *   if (motorX_pluse==1) { motorX_pluse=0; motorX_position++; }
     *   if (motorY_pluse==1) { motorY_pluse=0; motorY_position++; }
     *
     * ★ 本阶段**不迁入**：属 XY 插补层后续阶段。
     *   注意 Legacy 中 motorX/Y_pluse 由速度规划流程置位；
     *   V1 当前两者恒为 0 → 即使迁入，该分支实际也是 no-op。 */
  }
  /* TIM1（FOC Tick，Legacy USER/interru.c → test()）分支待 Control 层迁入后补上。 */
}