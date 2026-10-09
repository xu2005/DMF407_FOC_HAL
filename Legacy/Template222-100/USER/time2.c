#include "time2.h"
#include "stm32f4xx.h"
#include <stdint.h>
#include "angle.h"
#include "globals.h"

void Time2_Init(uint16_t arr, uint16_t psc)
{
	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
	TIM_InternalClockConfig(TIM2);
	
	TIM_TimeBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;
	TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up;//在此处进行转速和角度的获取
	TIM_TimeBaseInitStructure.TIM_Period = arr; //重载值
	TIM_TimeBaseInitStructure.TIM_Prescaler = psc;
	TIM_TimeBaseInitStructure.TIM_RepetitionCounter = 0;
	TIM_TimeBaseInit(TIM2, &TIM_TimeBaseInitStructure);
	
	 TIM_SetCounter(TIM2, 0);
	
	TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);
	 

	NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
	NVIC_Init(&NVIC_InitStructure);

	TIM_Cmd(TIM2, ENABLE);
	
}
// TIM2 中断服务函数，用于周期性更新电机状态
void TIM2_IRQHandler(void)
{
    if (TIM_GetITStatus(TIM2, TIM_IT_Update) != RESET)
    {
        // 清除中断标志
        TIM_ClearITPendingBit(TIM2, TIM_IT_Update);

        // 定义局部变量来缓存电角度和速度
        float local_theta;
        float motor_speed;

        // 调用函数计算电角度和电机速度
        calculate_motor_parameters(&local_theta, &motor_speed);

        // 更新全局变量或其他操作
        // 假设有全局变量 Speed_actual 和 theta，使用计算值更新
        theta = local_theta; 
        		// 更新电角度
        Speed_actual = motor_speed; // 更新电机速度
			
			
    }
}



void Time4_Init(uint16_t arr, uint16_t psc)
{
	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);
	NVIC_InitTypeDef NVIC_InitStructure;
	
	TIM_TimeBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;
	TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up;//在此处进行转速和角度的获取
	TIM_TimeBaseInitStructure.TIM_Period = arr; //重载值
	TIM_TimeBaseInitStructure.TIM_Prescaler = psc;
	TIM_TimeBaseInitStructure.TIM_RepetitionCounter = 0;
	TIM_TimeBaseInit(TIM4, &TIM_TimeBaseInitStructure);
	
	TIM_SetCounter(TIM4, 0);
	
	TIM_ITConfig(TIM4, TIM_IT_Update, ENABLE);
	NVIC_InitStructure.NVIC_IRQChannel = TIM4_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
	NVIC_Init(&NVIC_InitStructure);
	TIM_Cmd(TIM4, ENABLE);
	
}

extern u8 motorX_pluse;
extern u8 motorY_pluse;
long motorX_position=0,motorY_position=0;
void TIM4_IRQHandler(void)
{ 
  if (TIM_GetITStatus(TIM4, TIM_IT_Update) != RESET)
  {
    TIM_ClearITPendingBit(TIM4, TIM_IT_Update);
    if(motorX_pluse==1)  {motorX_pluse=0;motorX_position++;}
    if(motorY_pluse==1)  {motorY_pluse=0;motorY_position++;}
  }
}



