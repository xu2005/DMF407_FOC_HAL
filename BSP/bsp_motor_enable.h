/**
  ******************************************************************************
  * @file    bsp_motor_enable.h
  * @brief   PF10 / PM1_CTRL_SD 软件禁能命令（D-20 安全例外）
  *
  * 根据 ATK-PD6010B_V1.0 图纸，CTRL_SD=0 为板级目标禁止态。
  * 实板极性、R70、复位/高阻窗口及保护链仍未验收：POWER-STAGE ENABLE BLOCKED。
  * 本阶段仅提供禁能接口；无置高使能 API，无硬件安全反馈接口。
  ******************************************************************************
  */
#ifndef BSP_MOTOR_ENABLE_H
#define BSP_MOTOR_ENABLE_H

#ifdef __cplusplus
extern "C" {
#endif

/* 仅发送 PF10 Low 命令；不证明驱动板已关断，不改变 TIM1 PWM。
 * 调用前必须已完成 MX_GPIO_Init()（GPIOF 时钟及输出模式初始化）。 */
void BSP_MotorEnable_ForceDisable(void);

#ifdef __cplusplus
}
#endif

#endif /* BSP_MOTOR_ENABLE_H */
