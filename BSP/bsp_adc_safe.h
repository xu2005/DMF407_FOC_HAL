#ifndef BSP_ADC_SAFE_H
#define BSP_ADC_SAFE_H
#include <stdint.h>
#include "foc_pipeline.h"
int BSP_ADC_StartChecked(void);
/* Main context only (SysTick must run). Finite timeout PER conversion.
 * Retains W/V/BUS/W/V/BUS; publishes output only on complete success.
 * This changes V1 infinite-wait semantics, not ADC configuration. */
int BSP_ADC_ReadAllChecked(FOC_RawSample *sample, uint32_t timeout_ms);
#endif
