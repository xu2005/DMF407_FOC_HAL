#include "bsp_safety.h"
#include "bsp_motor_enable.h"
#include "bsp_pwm.h"
#include "tim.h"
#include <math.h>
static volatile BSP_Fault latched_fault;
static const uint32_t channels[] = { TIM_CHANNEL_1, TIM_CHANNEL_2, TIM_CHANNEL_3 };

static void inhibit_outputs(void)
{
    /* Board gate first, then hardware timer gates BEFORE any HAL call.
     * AOE must be off too: leaving it set could restore MOE on an update.
     * This is a V2 fault action, not a change to normal CubeMX BDTR config. */
    BSP_MotorEnable_ForceDisable();
    if (!htim1.Instance) { return; } /* Early init failure: TIM1 not configured. */
    __HAL_TIM_DISABLE_IT(&htim1, TIM_IT_UPDATE);
    htim1.Instance->BDTR &= ~(TIM_BDTR_MOE | TIM_BDTR_AOE);
    htim1.Instance->CCER &= ~(TIM_CCER_CC1E | TIM_CCER_CC1NE |
                             TIM_CCER_CC2E | TIM_CCER_CC2NE |
                             TIM_CCER_CC3E | TIM_CCER_CC3NE);
    htim1.Instance->CR1 &= ~TIM_CR1_CEN;
}
int BSP_PWM_StopChecked(void)
{
    int ok = 1;
    inhibit_outputs();
    if (!htim1.Instance) { return 1; }
    for (unsigned i=0; i<3; ++i) {
        if (HAL_TIM_PWM_Stop(&htim1, channels[i]) != HAL_OK) { ok=0; }
        if (HAL_TIMEx_PWMN_Stop(&htim1, channels[i]) != HAL_OK) { ok=0; }
    }
    /* Even HAL stop failures cannot leave modeled output enables asserted. */
    inhibit_outputs();
    if (!ok && latched_fault==BSP_FAULT_NONE) { latched_fault=BSP_FAULT_PWM_STOP; }
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
int BSP_PWM_StartChecked(void)
{
    BSP_MotorEnable_ForceDisable();
    if (latched_fault != BSP_FAULT_NONE) { return 0; }
    for (unsigned i=0; i<3; ++i) {
        if (HAL_TIM_PWM_Start(&htim1, channels[i]) != HAL_OK ||
            HAL_TIMEx_PWMN_Start(&htim1, channels[i]) != HAL_OK) {
            BSP_Safety_Fault(BSP_FAULT_PWM_START);
            return 0;
        }
    }
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
    BSP_MotorEnable_ForceDisable();
    BSP_PWM_SetABC((uint16_t)pwm->ta, (uint16_t)pwm->tb, (uint16_t)pwm->tc);
    return 1;
}
