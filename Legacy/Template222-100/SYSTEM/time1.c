#include "time1.h"
#include "stm32f4xx_gpio.h"

void TIM1_Int_Init(u16 arr,u16 psc)
{

    GPIO_InitTypeDef GPIO_InitStructure; //GPIO初始化
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure; //基结构体初始化
    TIM_OCInitTypeDef TIM_OCInitStructure;//输出结构体初始化
    TIM_BDTRInitTypeDef TIM_BDTRInitStructure;//定时器刹车和死区结构体初始化
     NVIC_InitTypeDef NVIC_InitStructure;//中断优先级的初始化
    //
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);//使能定时器端口和总线，高级定时器的时钟来自APB2 
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);//使能互补通道和接口
	  RCC_AHB1PeriphClockCmd( RCC_AHB1Periph_GPIOF, ENABLE);
     RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1,ENABLE);//TIM1 时钟使能
   
    //初始化所有引脚
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
   GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;  // 
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
   GPIO_InitStructure.GPIO_PuPd =GPIO_PuPd_DOWN;  // 
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOF, &GPIO_InitStructure);
//////	// 
//GPIO_ResetBits(GPIOF, GPIO_Pin_10); // X ????????
//////////	 
////// 
 GPIO_SetBits(GPIOF, GPIO_Pin_10); // X ????????
//	 

   //GPIO做相应设置，为AF输出  //PA8，PB13一组互补输出  PA9，PB14一组互补输出 A10，PB15一组互补输出
    //PA.8/9口设置为TIM1的OC1输出口
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_9| GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;  //复用功能
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;//推挽输出可以输出高电平和低电平
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    //PB.13/14/15口设置为TIM1_CH1N\TIM1_CH2N\输出口
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13 | GPIO_Pin_14| GPIO_Pin_15;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
		
		GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;
   GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
   GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL; // ?????????
   GPIO_Init(GPIOB, &GPIO_InitStructure);
//    GPIO_ResetBits(GPIOB, GPIO_Pin_12); // X ????????

//    GPIO_SetBits(GPIOB, GPIO_Pin_12);
    
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource8, GPIO_AF_TIM1);
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource9, GPIO_AF_TIM1);
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource10, GPIO_AF_TIM1);

    GPIO_PinAFConfig(GPIOB, GPIO_PinSource13, GPIO_AF_TIM1);
    GPIO_PinAFConfig(GPIOB, GPIO_PinSource14, GPIO_AF_TIM1);
    GPIO_PinAFConfig(GPIOB, GPIO_PinSource15, GPIO_AF_TIM1);

    //TIM1为高级定时器，Repetitioncounter仅设置高级定时器需要
    
    TIM_TimeBaseStructure.TIM_Period = arr; //自动重装载值
    TIM_TimeBaseStructure.TIM_Prescaler=psc; //定时器分频
//    TIM_TimeBaseStructure.TIM_CounterMode=TIM_CounterMode_Up; //向上计数模式
   TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_CenterAligned1;  //中央对齐模式1，从0计数到 TIM_Period 然后开始减到0，循环

    TIM_TimeBaseStructure.TIM_ClockDivision=TIM_CKD_DIV1;//时钟分频因子
    TIM_TimeBaseStructure.TIM_RepetitionCounter = 0;
    TIM_TimeBaseInit(TIM1,&TIM_TimeBaseStructure);// ②初始化定时器 TIM1
    TIM_ARRPreloadConfig(TIM1, ENABLE); //TIM1定时器的自动重装载寄存器的预装载功能
    
    
    //初始化 TIM1 PWM 模式
  //CNT<CCR1为有效电平
    //TIM1_OC1模块设置（设置1通道占空比）
    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;//TIM脉冲宽度调制模式1
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;//输出通道使能
    TIM_OCInitStructure.TIM_OutputNState = TIM_OutputNState_Enable;//互补输出
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
    TIM_OCInitStructure.TIM_OCNPolarity = TIM_OCNPolarity_High;//TIM输出比较极性高

    TIM_OCInitStructure.TIM_OCIdleState = TIM_OCIdleState_Reset;
    TIM_OCInitStructure.TIM_OCNIdleState = TIM_OCNIdleState_Reset;

    TIM_OC1Init(TIM1, &TIM_OCInitStructure);// 启用 CCR1 寄存器的影子寄存器

    //启用CCR1寄存器的影子寄存器（直到产生更新事件才更改设置）
    TIM_OC1PreloadConfig(TIM1, TIM_OCPreload_Enable);
    TIM_OC2Init(TIM1, &TIM_OCInitStructure);
    //启用CCR2寄存器的影子寄存器（直到产生更新事件才更改设置）
    TIM_OC2PreloadConfig(TIM1, TIM_OCPreload_Enable);
    //TIM1_OC3模块设置（设置3通道占空比）
    TIM_OC3Init(TIM1, &TIM_OCInitStructure);
    //启用CCR3寄存器的影子寄存器（直到产生更新事件才更改设置）
    TIM_OC3PreloadConfig(TIM1, TIM_OCPreload_Enable);
		

//		TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;//输出通道使能
//    TIM_OCInitStructure.TIM_OutputNState = TIM_OutputNState_Disable;//互补输出
////		TIM_OCInitStructure.TIM_Pulse = 50;// 设置占空比为50%
//    TIM_OCInitStructure.TIM_OCIdleState = TIM_OCIdleState_Reset;
//    TIM_OCInitStructure.TIM_OCNIdleState = TIM_OCNIdleState_Reset;
//		  TIM_OC4Init(TIM1, &TIM_OCInitStructure);
//			
//    //启用CCR3寄存器的影子寄存器（直到产生更新事件才更改设置）
//    TIM_OC4PreloadConfig(TIM1, TIM_OCPreload_Enable);
//		
     // 有关刹车和死区结构体的成员具体可参考BDTR寄存器的描述

    TIM_BDTRInitStructure.TIM_OSSRState = TIM_OSSRState_Disable;
    TIM_BDTRInitStructure.TIM_OSSIState = TIM_OSSIState_Disable;
    TIM_BDTRInitStructure.TIM_LOCKLevel = TIM_LOCKLevel_1;
    
    // 输出比较信号死区时间配置，具体如何计算可参考 BDTR:UTG[7:0]的描述
  // 这里配置的死区时间
       TIM_BDTRInitStructure.TIM_DeadTime = 84;
      TIM_BDTRInitStructure.TIM_Break = TIM_Break_Enable;
   // 当BKIN引脚检测到高电平的时候，输出比较信号被禁止，就好像是刹车一样
     TIM_BDTRInitStructure.TIM_BreakPolarity = TIM_BreakPolarity_High;
    TIM_BDTRInitStructure.TIM_AutomaticOutput = TIM_AutomaticOutput_Enable;
    TIM_BDTRConfig(TIM1, &TIM_BDTRInitStructure);
    
     NVIC_InitStructure.NVIC_IRQChannel = TIM1_UP_TIM10_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
    
     
     TIM_ClearFlag(TIM1, TIM_FLAG_Update);                                    //清除刹车中断，BKIN输入导致的中断
    TIM_ITConfig(TIM1, TIM_IT_Update, ENABLE);                               //使能刹车中断
  
  TIM_CtrlPWMOutputs(TIM1, ENABLE);                                        //PWM输出使能    

   
  TIM_SelectOutputTrigger(TIM1, TIM_TRGOSource_OC1Ref);
    TIM_Cmd(TIM1, ENABLE); //使能 TIM1 
 
}





