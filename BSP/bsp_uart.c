#include "bsp_uart.h"
#include "usart.h"

HAL_StatusTypeDef BSP_UART_SendByte(uint8_t data)
{
  return HAL_UART_Transmit(&huart1, &data, 1U, 10U);
}
