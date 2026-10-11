#ifndef HOST_MAIN_H
#define HOST_MAIN_H
#include "mock_hal.h"
typedef struct { uint32_t ODR; } GPIO_TypeDef;
typedef enum { GPIO_PIN_RESET=0, GPIO_PIN_SET=1 } GPIO_PinState;
extern GPIO_TypeDef mock_gpiof;
#define PM1_CTRL_SD_GPIO_Port (&mock_gpiof)
#define PM1_CTRL_SD_Pin 0x400U
void HAL_GPIO_WritePin(GPIO_TypeDef *port,uint16_t pin,GPIO_PinState state);
#endif
