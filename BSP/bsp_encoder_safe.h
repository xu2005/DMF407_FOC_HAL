#ifndef BSP_ENCODER_SAFE_H
#define BSP_ENCODER_SAFE_H
#include <stdint.h>
int BSP_EncoderSafe_Start(void);
int BSP_EncoderSafe_Sample(uint16_t *raw, int64_t *total);
void BSP_EncoderSafe_OverflowTick(void);
uint32_t BSP_EncoderSafe_GetOverflowEvents(void);
#endif
