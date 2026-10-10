/**
  ******************************************************************************
  * @file    bsp_encoder.c
  * @brief   TIM3 正交编码器的 BSP 实现
  *
  * Legacy 出处（基线 = Legacy/Template222_A/）：
  *   USER/encoderini.c:42   TIM_EncoderInterfaceConfig(TIM3, TI12, Rising, Rising)
  *   USER/encoderini.c:64   TIM_SetCounter(TIM3, 0)
  *   USER/encoderini.c:66   TIM_ClearFlag(TIM3, TIM_FLAG_Update)
  *   USER/encoderini.c:67   TIM_ITConfig(TIM3, TIM_IT_Update, ENABLE)
  *   USER/encoderini.c:68-69  CC1/CC2 中断被原作者**显式注释掉**
  *   USER/encoderini.c:74   TIM_Cmd(TIM3, ENABLE)
  *   USER/angle.c:15-32     TIM3_IRQHandler() 的 Update 分支
  *   USER/angle.c:37        gtim_get_encode()
  *
  * V1 约束（任务书 §9）：不改编码器极性、不改 Filter、不改 ARR/PSC、
  *                      不重新标定 4000 计数/转与 4 极对数。
  ******************************************************************************
  */

#include "bsp_encoder.h"
#include "tim.h"

#define BSP_ENCODER_TIM            (&htim3)
#define BSP_ENCODER_PERIOD_COUNTS  (65536)   /* ARR=65535 → 每次环绕 65536 计数 */

/* 对应 Legacy 全局量 g_timx_encode_count（globals.c，类型 int） */
static volatile int32_t s_encoder_overflow_count = 0;

void BSP_Encoder_Start(void)
{
  /* ★ 关键：不能用 HAL_TIM_Encoder_Start_IT()
   *
   * 已核实 Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_tim.c：
   *   HAL_TIM_Encoder_Start_IT()  → TIM_CCxChannelCmd(CH1/CH2) +
   *                                 __HAL_TIM_ENABLE_IT(TIM_IT_CC1) +
   *                                 __HAL_TIM_ENABLE_IT(TIM_IT_CC2) +
   *                                 __HAL_TIM_ENABLE()
   *   即会**额外开启 CC1IE / CC2IE**，且**并不开启 Update 中断**。
   *   HAL_TIM_Encoder_Start()     → 仅 TIM_CCxChannelCmd(CH1/CH2) + __HAL_TIM_ENABLE()
   *
   * Legacy 只开 Update 中断（encoderini.c:67），
   * 且 CC1/CC2 中断在 :68-69 被作者显式注释掉 —— 是明确的设计意图。
   * 故此处用 Encoder_Start() 使能通道与计数器，再单独开启 Update 中断。
   */

  /* Legacy :64  TIM_SetCounter(TIM3, 0) */
  __HAL_TIM_SET_COUNTER(BSP_ENCODER_TIM, 0);

  /* Legacy :74  TIM_Cmd(TIM3, ENABLE) —— 使能编码器通道与计数器，不开中断 */
  HAL_TIM_Encoder_Start(BSP_ENCODER_TIM, TIM_CHANNEL_ALL);

  /* Legacy :66  TIM_ClearFlag(TIM3, TIM_FLAG_Update) —— 先清掉残留标志，
   *             避免使能中断瞬间产生一次虚假的 Update 中断 */
  __HAL_TIM_CLEAR_FLAG(BSP_ENCODER_TIM, TIM_FLAG_UPDATE);

  /* Legacy :67  TIM_ITConfig(TIM3, TIM_IT_Update, ENABLE) —— 只开 Update */
  __HAL_TIM_ENABLE_IT(BSP_ENCODER_TIM, TIM_IT_UPDATE);
}

uint16_t BSP_Encoder_GetRawCount(void)
{
  /* Legacy angle.c:37 / :43  TIM_GetCounter(TIM3) */
  return (uint16_t)__HAL_TIM_GET_COUNTER(BSP_ENCODER_TIM);
}

uint8_t BSP_Encoder_IsCountingDown(void)
{
  /* Legacy angle.c:23  if (TIM3->CR1 & TIM_CR1_DIR) */
  return (__HAL_TIM_IS_TIM_COUNTING_DOWN(BSP_ENCODER_TIM) != 0U) ? 1U : 0U;
}

int32_t BSP_Encoder_GetOverflowCount(void)
{
  return s_encoder_overflow_count;
}

int32_t BSP_Encoder_GetCount(void)
{
  /* Legacy angle.c:37  gtim_get_encode()
   *   return (int32_t)(TIM_GetCounter(TIM3) + g_timx_encode_count * 65536); */
  return (int32_t)((int32_t)__HAL_TIM_GET_COUNTER(BSP_ENCODER_TIM)
                   + s_encoder_overflow_count * BSP_ENCODER_PERIOD_COUNTS);
}

void BSP_Encoder_OverflowTick(void)
{
  /* Legacy angle.c:23-30
   *   if (TIM3->CR1 & TIM_CR1_DIR) { g_timx_encode_count--; }
   *   else                         { g_timx_encode_count++; }
   *
   * 与 Legacy 一致：在中断里读取当时的方向位来判断本次是上溢还是下溢。
   * 硬件在 ARR=65535 处自动环绕，无需手动重置计数器。 */
  if (__HAL_TIM_IS_TIM_COUNTING_DOWN(BSP_ENCODER_TIM) != 0U)
  {
    s_encoder_overflow_count--;
  }
  else
  {
    s_encoder_overflow_count++;
  }
}