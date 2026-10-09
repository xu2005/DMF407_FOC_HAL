#ifndef __TIM2_H
#define __TIM2_H

#include <stdint.h>


// ��������
void Time2_Init(uint16_t arr, uint16_t psc);
void Time4_Init(uint16_t arr, uint16_t psc);
void TIM2_IRQHandler(void);
void TIM4_IRQHandler(void);
#endif
