#ifndef BSP_UART_H
#define BSP_UART_H

#include "stm32f4xx_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

/* MX_USART1_UART_Init() 后，从主循环发送一个原始字节。
 * 最长等待 10 ms，返回 HAL_OK / HAL_BUSY / HAL_TIMEOUT / HAL_ERROR。
 * 超时可能已发送部分数据，不自动重试；本轮无自动调用或接收 API。
 * Legacy UART 全部注释，本接口是用户明确要求的最小新增发送封装。 */
HAL_StatusTypeDef BSP_UART_SendByte(uint8_t data);

#ifdef __cplusplus
}
#endif

#endif /* BSP_UART_H */
