#include "globals.h"
#include "time1.h"
#include "stm32f4xx_gpio.h"
#include "test.h"
#include "current.h"
#include "adc.h"

void TIM1_UP_TIM10_IRQHandler(void) 
	{
    
		if (TIM_GetITStatus(TIM1, TIM_IT_Update) != RESET) 
			{
      TIM_ClearITPendingBit(TIM1, TIM_IT_Update);
       

				
				test();
             
				

    }
				
	}