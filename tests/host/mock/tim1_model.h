#ifndef TIM1_MODEL_H
#define TIM1_MODEL_H
#include "mock_hal.h"
/* Deliberately bounded model: BDTR first write/LOCK1, Break and Update/MOE.
 * No electrical, bus synchronization, ADC trigger or cycle-timing claim. */
typedef struct { TIM_TypeDef *regs; unsigned first_write, break_active; } Tim1_Model;
void Tim1_ModelReset(Tim1_Model *m, TIM_TypeDef *regs);
void Tim1_ModelWriteBDTR(Tim1_Model *m, uint32_t value);
void Tim1_ModelBreak(Tim1_Model *m, unsigned active);
void Tim1_ModelUpdate(Tim1_Model *m);
#endif
