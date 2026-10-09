/**
  ******************************************************************************
  * @file    bsp_pwm.h
  * @brief   TIM1 三相互补 PWM 的 BSP 封装
  *
  * 对应 Legacy: Template222_A/SYSTEM/time1.c (TIM1_Int_Init)
  *             Template222_A/USER/test.c  (TIM_SetCompare1/2/3)
  *
  * 职责边界（任务书 §4.3）：
  *   Control / Motor 层不得直接操作 TIM1 CCR，一律经本模块。
  *   本模块只做寄存器/外设访问，不含任何控制算法。
  ******************************************************************************
  */
#ifndef __BSP_PWM_H
#define __BSP_PWM_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* V1 待实测项 —— 实测前禁止作硬件导通状态断言
 *
 *   CCR = 0 时，六路输出（CH1/2/3 与 CH1N/2N/3N）的实际电平，
 *   以及它们对应的驱动板桥臂导通状态：
 *         NEEDS SCOPE CONFIRM
 *   必须在 V1 无功率示波器测试中确认（功率级直流侧不通电）。
 *   本文件不作任何"某侧必然导通/关断"的结论。
 */

/**
  * @brief  启动 TIM1 三相互补 PWM 输出
  * @note   Legacy TIM1 PWM 启动行为的 HAL 封装，
  *         **不是**与 TIM_CtrlPWMOutputs(TIM1, ENABLE) 的一一等价。
  *
  *         HAL_TIM_PWM_Start / HAL_TIMEx_PWMN_Start 一次涉及三件事：
  *           (1) CCx / CCxN 通道使能
  *           (2) MOE 主输出使能
  *           (3) TIM1 计数器使能（CEN）
  *         而 Legacy 的 TIM_CtrlPWMOutputs(TIM1, ENABLE) 仅对应其中的 MOE，
  *         Legacy 的计数器启动由 TIM_Cmd(TIM1, ENABLE) 单独完成。
  *
  *         故 TIM1 的启动顺序（PWM 输出 / 计数器 / Update IRQ）需在
  *         bsp_timer 接入时统一规划，不得重复初始化或引入新的控制行为。
  * @retval 无
  */
void BSP_PWM_Start(void);

/**
  * @brief  停止 TIM1 三相互补 PWM 输出
  * @note   注意：HAL 的 Stop 会一并关闭计数器（CEN），
  *         因此这是"停整组 PWM"，不是单独停某一路。
  *         用于 V1 无功率调试；V2 的故障急停需要独立设计。
  * @retval 无
  */
void BSP_PWM_Stop(void);

/**
  * @brief  更新三相占空比比较值
  * @note   等价 Legacy: TIM_SetCompare1/2/3(TIM1, tcm1/tcm2/tcm3)
  *         取值范围 0 ~ ARR(99)；V1 中 SVPWM 输出为浮点比较值，
  *         由调用方负责取整与限幅后再传入。
  * @param  u CH1 比较值（A 相）
  * @param  v CH2 比较值（B 相）
  * @param  w CH3 比较值（C 相）
  * @retval 无
  */
void BSP_PWM_SetABC(uint16_t u, uint16_t v, uint16_t w);

/**
  * @brief  读取当前三相占空比比较值
  * @note   调试/遥测用，不影响输出。
  * @param  u 输出 CH1 比较值
  * @param  v 输出 CH2 比较值
  * @param  w 输出 CH3 比较值
  * @retval 无
  */
void BSP_PWM_GetABC(uint16_t *u, uint16_t *v, uint16_t *w);

#ifdef __cplusplus
}
#endif

#endif /* __BSP_PWM_H */