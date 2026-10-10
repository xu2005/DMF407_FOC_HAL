#ifndef FOC_SVPWM_H
#define FOC_SVPWM_H

#include "foc_transforms.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int sector;
    float ta, tb, tc;
} FOC_SVPWMResult;

/* Legacy Ts=100, Udc=24; outputs retain its comparison-time units (not 0..1).
 * Return 1 for sectors 1..6. Return 0 for undefined-output sectors (zero => 0):
 * only sector is written, ta/tb/tc are NOT valid and must not be read or used.
 * This explicit status is a new interface difference, not a Legacy safety fix.
 * No TIM1/CCR access and no global tcm writes. */
int FOC_SVPWMCalculate(const FOC_AlphaBetaVoltage *input, FOC_SVPWMResult *result);

#ifdef __cplusplus
}
#endif

#endif
