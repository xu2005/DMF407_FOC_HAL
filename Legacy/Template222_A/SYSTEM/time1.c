#include "time1.h"
#include "stm32f4xx_gpio.h"

void TIM1_Int_Init(u16 arr,u16 psc)
{

    GPIO_InitTypeDef GPIO_InitStructure; //GPIO��ʼ��
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure; //���ṹ���ʼ��
    TIM_OCInitTypeDef TIM_OCInitStructure;//����ṹ���ʼ��
    TIM_BDTRInitTypeDef TIM_BDTRInitStructure;//��ʱ��ɲ���������ṹ���ʼ��
     NVIC_InitTypeDef NVIC_InitStructure;//�ж����ȼ��ĳ�ʼ��
    //
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);//ʹ�ܶ�ʱ���˿ں����ߣ��߼���ʱ����ʱ������APB2 
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);//ʹ�ܻ���ͨ���ͽӿ�
	  RCC_AHB1PeriphClockCmd( RCC_AHB1Periph_GPIOF, ENABLE);
     RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1,ENABLE);//TIM1 ʱ��ʹ��
   
    //��ʼ����������
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

   //GPIO����Ӧ���ã�ΪAF���  //PA8��PB13һ�黥�����  PA9��PB14һ�黥����� A10��PB15һ�黥�����
    //PA.8/9������ΪTIM1��OC1�����
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_9| GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;  //���ù���
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;//���������������ߵ�ƽ�͵͵�ƽ
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    //PB.13/14/15������ΪTIM1_CH1N\TIM1_CH2N\�����
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

    //TIM1Ϊ�߼���ʱ����Repetitioncounter�����ø߼���ʱ����Ҫ
    
    TIM_TimeBaseStructure.TIM_Period = arr; //�Զ���װ��ֵ
    TIM_TimeBaseStructure.TIM_Prescaler=psc; //��ʱ����Ƶ
//    TIM_TimeBaseStructure.TIM_CounterMode=TIM_CounterMode_Up; //���ϼ���ģʽ
   TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_CenterAligned1;  //�������ģʽ1����0������ TIM_Period Ȼ��ʼ����0��ѭ��

    TIM_TimeBaseStructure.TIM_ClockDivision=TIM_CKD_DIV1;//ʱ�ӷ�Ƶ����
    TIM_TimeBaseStructure.TIM_RepetitionCounter = 0;
    TIM_TimeBaseInit(TIM1,&TIM_TimeBaseStructure);// �ڳ�ʼ����ʱ�� TIM1
    TIM_ARRPreloadConfig(TIM1, ENABLE); //TIM1��ʱ�����Զ���װ�ؼĴ�����Ԥװ�ع���
    
    
    //��ʼ�� TIM1 PWM ģʽ
  //CNT<CCR1Ϊ��Ч��ƽ
    //TIM1_OC1ģ�����ã�����1ͨ��ռ�ձȣ�
    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;//TIM������ȵ���ģʽ1
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;//���ͨ��ʹ��
    TIM_OCInitStructure.TIM_OutputNState = TIM_OutputNState_Enable;//�������
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
    TIM_OCInitStructure.TIM_OCNPolarity = TIM_OCNPolarity_High;//TIM����Ƚϼ��Ը�

    TIM_OCInitStructure.TIM_OCIdleState = TIM_OCIdleState_Reset;
    TIM_OCInitStructure.TIM_OCNIdleState = TIM_OCNIdleState_Reset;

    TIM_OC1Init(TIM1, &TIM_OCInitStructure);// ���� CCR1 �Ĵ�����Ӱ�ӼĴ���

    //����CCR1�Ĵ�����Ӱ�ӼĴ�����ֱ�����������¼��Ÿ������ã�
    TIM_OC1PreloadConfig(TIM1, TIM_OCPreload_Enable);
    TIM_OC2Init(TIM1, &TIM_OCInitStructure);
    //����CCR2�Ĵ�����Ӱ�ӼĴ�����ֱ�����������¼��Ÿ������ã�
    TIM_OC2PreloadConfig(TIM1, TIM_OCPreload_Enable);
    //TIM1_OC3ģ�����ã�����3ͨ��ռ�ձȣ�
    TIM_OC3Init(TIM1, &TIM_OCInitStructure);
    //����CCR3�Ĵ�����Ӱ�ӼĴ�����ֱ�����������¼��Ÿ������ã�
    TIM_OC3PreloadConfig(TIM1, TIM_OCPreload_Enable);
		

//		TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;//���ͨ��ʹ��
//    TIM_OCInitStructure.TIM_OutputNState = TIM_OutputNState_Disable;//�������
////		TIM_OCInitStructure.TIM_Pulse = 50;// ����ռ�ձ�Ϊ50%
//    TIM_OCInitStructure.TIM_OCIdleState = TIM_OCIdleState_Reset;
//    TIM_OCInitStructure.TIM_OCNIdleState = TIM_OCNIdleState_Reset;
//		  TIM_OC4Init(TIM1, &TIM_OCInitStructure);
//			
//    //����CCR3�Ĵ�����Ӱ�ӼĴ�����ֱ�����������¼��Ÿ������ã�
//    TIM_OC4PreloadConfig(TIM1, TIM_OCPreload_Enable);
//		
     // �й�ɲ���������ṹ��ĳ�Ա����ɲο�BDTR�Ĵ���������

    TIM_BDTRInitStructure.TIM_OSSRState = TIM_OSSRState_Disable;
    TIM_BDTRInitStructure.TIM_OSSIState = TIM_OSSIState_Disable;
    TIM_BDTRInitStructure.TIM_LOCKLevel = TIM_LOCKLevel_1;
    
    // ����Ƚ��ź�����ʱ�����ã�������μ���ɲο� BDTR:UTG[7:0]������
  // �������õ�����ʱ��
    TIM_BDTRInitStructure.TIM_DeadTime = 84;
    TIM_BDTRInitStructure.TIM_Break = TIM_Break_Enable;
    // ��BKIN���ż�⵽�ߵ�ƽ��ʱ������Ƚ��źű���ֹ���ͺ�����ɲ��һ��
    TIM_BDTRInitStructure.TIM_BreakPolarity = TIM_BreakPolarity_High;
    TIM_BDTRInitStructure.TIM_AutomaticOutput = TIM_AutomaticOutput_Enable;
    TIM_BDTRConfig(TIM1, &TIM_BDTRInitStructure);
    
     NVIC_InitStructure.NVIC_IRQChannel = TIM1_UP_TIM10_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
    
     
     TIM_ClearFlag(TIM1, TIM_FLAG_Update);                                    //���ɲ���жϣ�BKIN���뵼�µ��ж�
    TIM_ITConfig(TIM1, TIM_IT_Update, ENABLE);                               //ʹ��ɲ���ж�
  
  TIM_CtrlPWMOutputs(TIM1, ENABLE);                                        //PWM���ʹ��    

   
  TIM_SelectOutputTrigger(TIM1, TIM_TRGOSource_OC1Ref);
    TIM_Cmd(TIM1, ENABLE); //ʹ�� TIM1 
 
}


