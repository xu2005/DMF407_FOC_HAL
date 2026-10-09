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
	TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up;//�ڴ˴�����ת�ٺͽǶȵĻ�ȡ
	TIM_TimeBaseInitStructure.TIM_Period = arr; //����ֵ
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
// TIM2 �жϷ����������������Ը��µ��״̬
void TIM2_IRQHandler(void)
{
    if (TIM_GetITStatus(TIM2, TIM_IT_Update) != RESET)
    {
        // ����жϱ�־
        TIM_ClearITPendingBit(TIM2, TIM_IT_Update);

        // ����ֲ������������ǶȺ��ٶ�
        float local_theta;
        float motor_speed;

        // ���ú��������ǶȺ͵���ٶ�
        calculate_motor_parameters(&local_theta, &motor_speed);

        // ����ȫ�ֱ�������������
        // ������ȫ�ֱ��� Speed_actual �� theta��ʹ�ü���ֵ����
        theta = local_theta; 
        		// ���µ�Ƕ�
        Speed_actual = motor_speed; // ���µ���ٶ�
			
			
    }
}



void Time4_Init(uint16_t arr, uint16_t psc)
{
	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);
	NVIC_InitTypeDef NVIC_InitStructure;
	
	TIM_TimeBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;
	TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up;//�ڴ˴�����ת�ٺͽǶȵĻ�ȡ
	TIM_TimeBaseInitStructure.TIM_Period = arr; //����ֵ
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

 



