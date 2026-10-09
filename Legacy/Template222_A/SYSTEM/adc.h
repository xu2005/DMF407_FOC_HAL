#ifndef __ADC_H
#define __ADC_H

#include "stm32f4xx.h"



// º¯ÊýÉùÃ÷

uint16_t ADC_GetValue(ADC_TypeDef* ADCx, uint8_t channel);
void ADC_ProcessData(void);
void ADC_Int_Init(void);
void ADC_IRQHandler(void);

#endif
