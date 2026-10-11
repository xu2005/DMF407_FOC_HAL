#ifndef BSP_SAFETY_H
#define BSP_SAFETY_H
#include <stdint.h>
#include "foc_svpwm.h"
typedef enum {
    BSP_FAULT_NONE = 0, BSP_FAULT_INIT = 1, BSP_FAULT_ADC_START = 2,
    BSP_FAULT_ADC_READ = 3, BSP_FAULT_PWM_START = 4, BSP_FAULT_PWM_STOP = 5,
    BSP_FAULT_CONTROL = 6, BSP_FAULT_ENCODER = 7, BSP_FAULT_STALE = 8,
    BSP_FAULT_TIMER = 9, BSP_FAULT_TIM1_CONFIG = 10
} BSP_Fault;
/* Faults latch until MCU reset. No power enable or automatic restart API. */
void BSP_Safety_Fault(BSP_Fault fault);
BSP_Fault BSP_Safety_GetFault(void);
int BSP_TIM1_ConfigChecked(void);
int BSP_TIM1_StartDiagnosticChecked(void);
int BSP_PWM_StartChecked(void);
int BSP_PWM_StopChecked(void);
int BSP_PWM_ApplyChecked(const FOC_SVPWMResult *pwm);
#endif
