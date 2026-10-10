/**
  ******************************************************************************
  * @file    bsp_motor_enable.c
  * @brief   PF10 禁能写入的 BSP 封装
  *
  * Legacy SYSTEM/time1.c:29 默认置高；本轮明确选择安全优先（D-20）。
  * 最早的输出锁存位 Low 由 CubeMX 生成的 MX_GPIO_Init() 预置；
  * 本接口只作重复禁能，不能代替 .ioc 初始 Low 或外部硬件保护。
  ******************************************************************************
  */

#include "bsp_motor_enable.h"
#include "main.h"

void BSP_MotorEnable_ForceDisable(void)
{
  HAL_GPIO_WritePin(PM1_CTRL_SD_GPIO_Port, PM1_CTRL_SD_Pin, GPIO_PIN_RESET);
}
