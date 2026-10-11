#include "bsp_safety.h"
#include "bsp_motor_enable.h"
#include "bsp_pwm.h"
#include "tim.h"
#include <math.h>
static volatile BSP_Fault latched_fault;
static const uint32_t channels[] = { TIM_CHANNEL_1, TIM_CHANNEL_2, TIM_CHANNEL_3 };

static void inhibit_outputs(void)
{
    /* AOE is locked at zero during initialization. Never try to clear it
     * after LOCK=1. MOE and CCxE/NE remain writable (RM0090 17.4.18).
     * An inherited locked AOE=1 is rejected at startup; requires reset. */
    BSP_MotorEnable_ForceDisable();
    if (!htim1.Instance) { return; } /* Early init failure: TIM1 not configured. */
    __HAL_TIM_DISABLE_IT(&htim1, TIM_IT_UPDATE);
    CLEAR_BIT(htim1.Instance->BDTR, TIM_BDTR_MOE);
    htim1.Instance->CCER &= ~(TIM_CCER_CC1E | TIM_CCER_CC1NE |
                             TIM_CCER_CC2E | TIM_CCER_CC2NE |
                             TIM_CCER_CC3E | TIM_CCER_CC3NE);
    htim1.Instance->CR1 &= ~TIM_CR1_CEN;
}
int BSP_PWM_StopChecked(void)
{
    uint32_t mask = __get_PRIMASK();
    __disable_irq();
    int ok = 1;
    inhibit_outputs();
    if (!htim1.Instance) { __set_PRIMASK(mask); return 1; }
    for (unsigned i=0; i<3; ++i) {
        if (HAL_TIM_PWM_Stop(&htim1, channels[i]) != HAL_OK) { ok=0; }
        if (HAL_TIMEx_PWMN_Stop(&htim1, channels[i]) != HAL_OK) { ok=0; }
    }
    /* Even HAL stop failures cannot leave modeled output enables asserted. */
    inhibit_outputs();
    if (!ok && latched_fault==BSP_FAULT_NONE) { latched_fault=BSP_FAULT_PWM_STOP; }
    __set_PRIMASK(mask);
    return ok;
}
void BSP_Safety_Fault(BSP_Fault fault)
{
    /* Serialize main/IRQ fault transitions and preserve the first reason. */
    uint32_t mask = __get_PRIMASK();
    __disable_irq();
    if (latched_fault == BSP_FAULT_NONE) { latched_fault = fault == BSP_FAULT_NONE ? BSP_FAULT_CONTROL : fault; }
    (void)BSP_PWM_StopChecked();
    __set_PRIMASK(mask);
}
BSP_Fault BSP_Safety_GetFault(void) { return latched_fault; }
int BSP_TIM1_ConfigChecked(void)
{
    if (latched_fault != BSP_FAULT_NONE) { return 0; }
    if (!htim1.Instance) { BSP_Safety_Fault(BSP_FAULT_INIT); return 0; }
    uint32_t bdtr = htim1.Instance->BDTR;
    if ((bdtr & TIM_BDTR_AOE) || (bdtr & TIM_BDTR_LOCK) != TIM_LOCKLEVEL_1) {
        BSP_Safety_Fault(BSP_FAULT_TIM1_CONFIG); return 0;
    }
    return 1;
}
int BSP_TIM1_StartDiagnosticChecked(void)
{
    /* OFF starts only the internal timebase. No PWM/PWMN HAL Start calls.
     * Internal CC1/ADC first-trigger behavior remains an L4 measurement. */
    uint32_t mask = __get_PRIMASK();
    __disable_irq();
    int ok = BSP_TIM1_ConfigChecked();
    if (ok) {
        inhibit_outputs();
        __HAL_TIM_CLEAR_FLAG(&htim1, TIM_FLAG_UPDATE);
        __HAL_TIM_ENABLE_IT(&htim1, TIM_IT_UPDATE);
        htim1.Instance->CR1 |= TIM_CR1_CEN;
    }
    __set_PRIMASK(mask);
    return ok;
}
int BSP_PWM_StartChecked(void)
{
    BSP_MotorEnable_ForceDisable();
    /* Prevent a fault ISR between the latch check and a HAL MOE write.
     * These six nonblocking HAL starts run only during main startup. */
    uint32_t mask = __get_PRIMASK();
    __disable_irq();
    if (!BSP_TIM1_ConfigChecked()) { __set_PRIMASK(mask); return 0; }
    for (unsigned i=0; i<3; ++i) {
        if (HAL_TIM_PWM_Start(&htim1, channels[i]) != HAL_OK ||
            HAL_TIMEx_PWMN_Start(&htim1, channels[i]) != HAL_OK) {
            BSP_Safety_Fault(BSP_FAULT_PWM_START);
            __set_PRIMASK(mask);
            return 0;
        }
    }
    __set_PRIMASK(mask);
    return 1;
}
int BSP_PWM_ApplyChecked(const FOC_SVPWMResult *pwm)
{
    if (latched_fault != BSP_FAULT_NONE) { return 0; }
    if (!htim1.Instance) { BSP_Safety_Fault(BSP_FAULT_INIT); return 0; }
    float limit = (float)htim1.Instance->ARR;
    if (!pwm || pwm->sector<1 || pwm->sector>6 || !isfinite(pwm->ta) ||
        !isfinite(pwm->tb) || !isfinite(pwm->tc) || pwm->ta<0 || pwm->tb<0 ||
        pwm->tc<0 || pwm->ta>limit || pwm->tb>limit || pwm->tc>limit || limit>65535) {
        BSP_Safety_Fault(BSP_FAULT_CONTROL);
        return 0;
    }
    uint32_t mask = __get_PRIMASK();
    __disable_irq();
    if (latched_fault != BSP_FAULT_NONE) { __set_PRIMASK(mask); return 0; }
    BSP_MotorEnable_ForceDisable();
    BSP_PWM_SetABC((uint16_t)pwm->ta, (uint16_t)pwm->tb, (uint16_t)pwm->tc);
    __set_PRIMASK(mask);
    return 1;
}
