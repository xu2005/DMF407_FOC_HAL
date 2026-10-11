#ifndef HOST_TIM_H
#define HOST_TIM_H
#include "mock_hal.h"
extern TIM_HandleTypeDef htim1, htim2, htim3, htim4;
#define TIM1 (htim1.Instance)
#define TIM2 (htim2.Instance)
#define TIM3 (htim3.Instance)
#define TIM4 (htim4.Instance)
#endif
