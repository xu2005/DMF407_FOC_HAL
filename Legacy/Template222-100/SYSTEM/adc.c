#include "stm32f4xx_gpio.h"
#include "adc.h"
#include "time1.h"
#include "globals.h"
#include "current.h"

void ADC_Int_Init(void)
{
       GPIO_InitTypeDef GPIO_InitStructure;
    ADC_InitTypeDef  ADC_InitStructure;
    ADC_CommonInitTypeDef ADC_CommonInitStructure; // 设置ADC的通用控制寄存器CCR，配置ADC输入时钟分频，模式为独立模式等。
    NVIC_InitTypeDef NVIC_InitStructure; // NVIC中断配置
	

    // 使能GPIOA和GPIOB时钟
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);

    // 使能ADC1时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC2, ENABLE);
	  RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC3, ENABLE);
    // 初始化GPIOA的PA3和PA6为模拟输入
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3 | GPIO_Pin_6;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AN; // 对模拟信号采样，将引脚设置为模拟模式
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    // 初始化GPIOB的PB0为模拟输入（假设你希望初始化PB0）
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

		
    // 初始化ADC通用配置
    ADC_CommonInitStructure.ADC_Mode = ADC_Mode_Independent; 
    ADC_CommonInitStructure.ADC_TwoSamplingDelay = ADC_TwoSamplingDelay_5Cycles; // 两个采样之间的延迟
    ADC_CommonInitStructure.ADC_DMAAccessMode = ADC_DMAAccessMode_Disabled;
    ADC_CommonInitStructure.ADC_Prescaler = ADC_Prescaler_Div4; // 保证ADC1的时钟频率不超过36MHz
    ADC_CommonInit(&ADC_CommonInitStructure);

    // 初始化ADC1
    ADC_InitStructure.ADC_Resolution = ADC_Resolution_12b; // 分辨率设为12位
    ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right; // 右对齐
   
		ADC_InitStructure.ADC_ContinuousConvMode = ENABLE;//DISABLE; // 不使用连续转换模式
    ADC_InitStructure.ADC_ScanConvMode =DISABLE ; // 开启扫描模式
    ADC_InitStructure.ADC_NbrOfConversion = 1;  // 使用1个通道进行采样
    ADC_InitStructure.ADC_ExternalTrigConvEdge =  ADC_ExternalTrigConvEdge_Falling; // 外部触发
    ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_T1_CC1; // 选择触发源（如有需要）
    
   ADC_RegularChannelConfig(ADC3, ADC_Channel_3, 1,  ADC_SampleTime_480Cycles); // PA3 -> ADC_Channel_3，w相
    ADC_RegularChannelConfig(ADC2, ADC_Channel_6, 1, ADC_SampleTime_480Cycles); // PA6 -> ADC_Channel_6，v相
    ADC_RegularChannelConfig(ADC1, ADC_Channel_9, 1, ADC_SampleTime_480Cycles); // PB0 -> ADC_Channel_9，母线电压
		 ADC_Init(ADC1, &ADC_InitStructure); 
		  ADC_Init(ADC2, &ADC_InitStructure); 
			  ADC_Init(ADC3, &ADC_InitStructure);
//		  NVIC_InitStructure.NVIC_IRQChannel = ADC_IRQn; // 选择ADC中断通道
//    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
//    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
//    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
//    NVIC_Init(&NVIC_InitStructure);
    // 使能 ADC1
    ADC_Cmd(ADC1, ENABLE);
		ADC_Cmd(ADC2, ENABLE);
		ADC_Cmd(ADC3, ENABLE);

 
}


uint16_t ADC_GetValue(ADC_TypeDef* ADCx, uint8_t channel) 
{
    // 选择通道
    ADC_RegularChannelConfig(ADCx, channel, 1, ADC_SampleTime_480Cycles);
    
    // 启动ADC转换
//    ADC_SoftwareStartConv(ADCx); // 启动ADC软件转换
//    
    // 等待转换完成
    while (ADC_GetFlagStatus(ADCx, ADC_FLAG_EOC) == RESET);  // 等待转换结束

    // 获取转换结果
    return ADC_GetConversionValue(ADCx);
}




    
    // 在此处处理采样数据
void ADC_ProcessData(void)
{
		 ADC_GetValue(ADC3, ADC_Channel_3);
		 ADC_GetValue(ADC2, ADC_Channel_6);
		 ADC_GetValue(ADC1, ADC_Channel_9);
	
    // 单次采样W相电流、V相电流和母线电压
    adc3_value = ADC_GetValue(ADC3, ADC_Channel_3); // 采样W相电流
    adc2_value = ADC_GetValue(ADC2, ADC_Channel_6); // 采样V相电流
    adc1_value = ADC_GetValue(ADC1, ADC_Channel_9); // 采样母线电压
    
    // 在此处处理采样数据
}


