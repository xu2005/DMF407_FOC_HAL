#ifndef __ANGLE_H
#define __ANGLE_H
#include <stdint.h>
//#include "stm32f4xx.h"

// 定义 DQ_Def 结构体，包含 d 和 q 分量
void TIM3_IRQHandler(void);
int32_t gtim_get_encode(void);
// IPARK 函数原型声明，用于执行逆Park变换
//float angle_cal(void);
//float calculate_motor_speed(void);
//void TIM2_IRQHandler(void);
void calculate_motor_parameters(float* electrical_angle, float* motor_speed);
#endif  //  结束头文件保护