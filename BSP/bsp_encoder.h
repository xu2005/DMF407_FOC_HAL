/**
  ******************************************************************************
  * @file    bsp_encoder.h
  * @brief   TIM3 正交编码器（PM1）的 BSP 封装
  *
  * 对应 Legacy（基线 = Legacy/Template222_A/）：
  *   USER/encoderini.c — Encoder_Int_Init()
  *   USER/angle.c      — TIM3_IRQHandler() 的 Update 分支、gtim_get_encode()
  *
  * 职责边界（任务书 §4.3）：
  *   本模块只做编码器外设访问与计数管理。
  *   机械角、电角度、转速换算属 angle.c，**尚未迁入**（归 Motor/Control 层）。
  ******************************************************************************
  */
#ifndef __BSP_ENCODER_H
#define __BSP_ENCODER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* 引脚映射（固定，来自 .ioc 与《引脚时钟配置规范》§7）
 *
 *   PC6 = TIM3_CH1 = PM1_ENCA
 *   PC7 = TIM3_CH2 = PM1_ENCB
 *
 *   TIM3：Encoder TI12 / PSC=0 / ARR=65535 / IC1,IC2 Filter=4 / ARPE=Disable
 */

/* V1 待实测项 —— 实测前禁止作结论
 *
 *   (1) 计数方向（正转递增还是递减）        = NEEDS MEASURE
 *   (2) 机械转一圈的计数增量               = NEEDS MEASURE
 *   (3) 16 位计数器溢出/下溢时的扩展计数正确性 = NEEDS MEASURE
 *   (4) 编码器极性（Legacy 为 Rising/Rising）= 已按 Legacy 保持，未重新标定
 *
 *   注：Legacy angle.c 中的 4000 计数/转、4 极对数、角度偏移 0.546637118
 *       全部保留为待迁移基线，本模块**不涉及**这些换算。
 */

/**
  * @brief  启动 TIM3 编码器接口与 Update 中断
  * @note   对应 Legacy encoderini.c:64/66/67/74：
  *           TIM_SetCounter(TIM3, 0) / TIM_ClearFlag(Update) /
  *           TIM_ITConfig(Update, ENABLE) / TIM_Cmd(TIM3, ENABLE)
  *         **仅开启 Update 中断**，不开 CC1/CC2 中断。
  * @retval 无
  */
void BSP_Encoder_Start(void);

/**
  * @brief  读取 16 位原始计数值
  * @note   对应 Legacy TIM_GetCounter(TIM3)
  * @retval 0 ~ 65535
  */
uint16_t BSP_Encoder_GetRawCount(void);

/**
  * @brief  读取当前计数方向
  * @note   对应 Legacy `TIM3->CR1 & TIM_CR1_DIR`
  * @retval 1 = 向下计数，0 = 向上计数
  */
uint8_t BSP_Encoder_IsCountingDown(void);

/**
  * @brief  读取溢出/下溢累计次数
  * @note   对应 Legacy 全局量 g_timx_encode_count（每次上溢 +1、下溢 −1）
  * @retval 有符号累计次数
  */
int32_t BSP_Encoder_GetOverflowCount(void);

/**
  * @brief  读取扩展后的 32 位计数
  * @note   对应 Legacy angle.c:37 gtim_get_encode()：
  *           (int32_t)(TIM_GetCounter(TIM3) + g_timx_encode_count * 65536)
  * @retval 扩展计数
  */
int32_t BSP_Encoder_GetCount(void);

/**
  * @brief  Update 事件处理（上溢/下溢时调整扩展计数）
  * @note   对应 Legend angle.c:15-32 TIM3_IRQHandler() 的 Update 分支。
  *         由 HAL_TIM_PeriodElapsedCallback() 在 TIM3 分支调用，
  *         业务逻辑不直接写在 stm32f4xx_it.c（任务书 §8）。
  * @retval 无
  */
void BSP_Encoder_OverflowTick(void);

#ifdef __cplusplus
}
#endif

#endif /* __BSP_ENCODER_H */