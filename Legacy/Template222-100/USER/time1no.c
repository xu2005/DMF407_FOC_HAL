#include "time1no.h"
#include "stm32f4xx.h"
#include <stdint.h>  // 确保包含这个头文件
void TIM1_NoOutputPWM_Init(uint16_t arr, uint16_t psc)
{
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    TIM_OCInitTypeDef TIM_OCInitStructure;

    // TIM1 基础配置
    TIM_TimeBaseStructure.TIM_Period = arr;   // 自动重装载值
    TIM_TimeBaseStructure.TIM_Prescaler = psc; // 预分频器
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseStructure.TIM_CounterMode =TIM_CounterMode_CenterAligned1;
    TIM_TimeBaseInit(TIM1, &TIM_TimeBaseStructure);

    // 初始化 PWM 模式，不输出信号
    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Disable; // 禁用输出状态
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;

    // 配置通道1，没有输出
    TIM_OC1Init(TIM1, &TIM_OCInitStructure);
    TIM_OC1PreloadConfig(TIM1, TIM_OCPreload_Enable);

    // 配置通道2，没有输出
    TIM_OC2Init(TIM1, &TIM_OCInitStructure);
    TIM_OC2PreloadConfig(TIM1, TIM_OCPreload_Enable);

    // 配置通道3，没有输出
    TIM_OC3Init(TIM1, &TIM_OCInitStructure);
    TIM_OC3PreloadConfig(TIM1, TIM_OCPreload_Enable);

    // 使能 TIM1 定时器
    TIM_Cmd(TIM1, ENABLE);
}
