#include "encoderini.h"
#include "stm32f4xx_gpio.h"
#include "stm32f4xx_tim.h"
#include "stm32f4xx_rcc.h"

void Encoder_Int_Init(u16 arr, u16 psc)
{
    GPIO_InitTypeDef GPIO_InitStructure;  // GPIO
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;  // ?
    TIM_ICInitTypeDef TIM_ICInitStructure;  // 编码器输入通道初始化
    NVIC_InitTypeDef NVIC_InitStructure;  // NVIC ??????

    // ?? GPIOC ? TIM3 ???
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);

    // ?? GPIOC ?? 6 ? 7 ???????,?? TIM3 ??????
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	 GPIO_InitStructure.GPIO_PuPd =   GPIO_PuPd_NOPULL;  // 设置为上拉模式，初始为高电平
    GPIO_Init(GPIOC, &GPIO_InitStructure);
//// 
//    GPIO_SetBits(GPIOC, GPIO_Pin_5);

//    // 设置 B 相为低电平
//    GPIO_ResetBits(GPIOC, GPIO_Pin_7);//编码器实现正反转，可以通过配置AB相的初始化电平
//    // ?? GPIO ????
    GPIO_PinAFConfig(GPIOC, GPIO_PinSource6, GPIO_AF_TIM3);
    GPIO_PinAFConfig(GPIOC, GPIO_PinSource7, GPIO_AF_TIM3);

    // ?????? TIM3
    TIM_TimeBaseStructure.TIM_Period = arr;  // ??????
    TIM_TimeBaseStructure.TIM_Prescaler = psc;  
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;  
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;  // ????
    TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);  // ??? TIM3

    // ?? TIM3 ????????,?? A/B ???,?????
 
   
		 TIM_EncoderInterfaceConfig(TIM3, TIM_EncoderMode_TI12, TIM_ICPolarity_Rising,  TIM_ICPolarity_Rising );
     TIM_ICStructInit(&TIM_ICInitStructure);

    TIM_ICInitStructure.TIM_ICFilter =4;  // ?????????
   TIM_ICInitStructure.TIM_Channel = TIM_Channel_1;
	 TIM_ICInitStructure.TIM_ICSelection = TIM_ICSelection_DirectTI; // 映射到 TI1 上
     TIM_ICInitStructure.TIM_ICPrescaler = TIM_ICPSC_DIV1; // 配置输入分频,不分频
    TIM_ICInit(TIM3, &TIM_ICInitStructure);
   

    TIM_ICInitStructure.TIM_Channel = TIM_Channel_2;
		TIM_ICInitStructure.TIM_ICSelection = TIM_ICSelection_DirectTI; // 映射到 TI1 上
    TIM_ICInitStructure.TIM_ICPrescaler = TIM_ICPSC_DIV1; // 配置输入分频,不分频
    TIM_ICInit(TIM3, &TIM_ICInitStructure);
	 

    // ?? NVIC ??
    NVIC_InitStructure.NVIC_IRQChannel = TIM3_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
		TIM_SetCounter(TIM3, 0);
		
     TIM_ClearFlag(TIM3, TIM_FLAG_Update);     
    TIM_ITConfig(TIM3, TIM_IT_Update, ENABLE);
//   TIM_ITConfig(TIM3, TIM_IT_CC1, ENABLE);
//	   TIM_ITConfig(TIM3, TIM_IT_CC2, ENABLE);
		 
		 
		 
    // ?? TIM3 ???
    TIM_Cmd(TIM3, ENABLE);
		
}
