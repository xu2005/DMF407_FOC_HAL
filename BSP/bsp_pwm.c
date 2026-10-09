/**
  ******************************************************************************
  * @file    bsp_pwm.c
  * @brief   TIM1 三相互补 PWM 的 BSP 实现
  *
  * Legacy 出处（基线 = Legacy/Template222_A/）：
  *   SYSTEM/time1.c  — TIM1_Int_Init()：时基、六路 PWM、BDTR、TRGO
  *   USER/test.c     — TIM_SetCompare1/2/3(TIM1, tcm1/tcm2/tcm3)
  *
  * V1 约束（任务书 §9）：不改 PWM 频率、不改死区、不改初始 CCR。
  * 因此本模块只做"启动/停止/写比较值"，不重新配置 TIM1 时基。
  ******************************************************************************
  */

#include "bsp_pwm.h"
#include "tim.h"

/* TIM1 六路输出通道：CH1/2/3 为高侧，CH1N/2N/3N 为互补低侧 */
#define BSP_PWM_CH_U    TIM_CHANNEL_1   /* Legacy: tcm1 → TIM_SetCompare1 */
#define BSP_PWM_CH_V    TIM_CHANNEL_2   /* Legacy: tcm2 → TIM_SetCompare2 */
#define BSP_PWM_CH_W    TIM_CHANNEL_3   /* Legacy: tcm3 → TIM_SetCompare3 */

void BSP_PWM_Start(void)
{
  /* Legacy TIM1 PWM 启动行为的 HAL 封装（非 TIM_CtrlPWMOutputs 的一一等价）：
   * 除 MOE 外还一并使能 CCx/CCxN 与计数器 CEN，详见 bsp_pwm.h 说明。
   * TIM1 启动顺序由 bsp_timer 统一规划。 */
  HAL_TIM_PWM_Start(&htim1, BSP_PWM_CH_U);
  HAL_TIMEx_PWMN_Start(&htim1, BSP_PWM_CH_U);

  HAL_TIM_PWM_Start(&htim1, BSP_PWM_CH_V);
  HAL_TIMEx_PWMN_Start(&htim1, BSP_PWM_CH_V);

  HAL_TIM_PWM_Start(&htim1, BSP_PWM_CH_W);
  HAL_TIMEx_PWMN_Start(&htim1, BSP_PWM_CH_W);
}

void BSP_PWM_Stop(void)
{
  HAL_TIM_PWM_Stop(&htim1, BSP_PWM_CH_U);
  HAL_TIMEx_PWMN_Stop(&htim1, BSP_PWM_CH_U);

  HAL_TIM_PWM_Stop(&htim1, BSP_PWM_CH_V);
  HAL_TIMEx_PWMN_Stop(&htim1, BSP_PWM_CH_V);

  HAL_TIM_PWM_Stop(&htim1, BSP_PWM_CH_W);
  HAL_TIMEx_PWMN_Stop(&htim1, BSP_PWM_CH_W);
}

void BSP_PWM_SetABC(uint16_t u, uint16_t v, uint16_t w)
{
  /* 等价 Legacy: TIM_SetCompare1/2/3(TIM1, tcm1/tcm2/tcm3) */
  __HAL_TIM_SET_COMPARE(&htim1, BSP_PWM_CH_U, u);
  __HAL_TIM_SET_COMPARE(&htim1, BSP_PWM_CH_V, v);
  __HAL_TIM_SET_COMPARE(&htim1, BSP_PWM_CH_W, w);
}

void BSP_PWM_GetABC(uint16_t *u, uint16_t *v, uint16_t *w)
{
  *u = (uint16_t)__HAL_TIM_GET_COMPARE(&htim1, BSP_PWM_CH_U);
  *v = (uint16_t)__HAL_TIM_GET_COMPARE(&htim1, BSP_PWM_CH_V);
  *w = (uint16_t)__HAL_TIM_GET_COMPARE(&htim1, BSP_PWM_CH_W);
}