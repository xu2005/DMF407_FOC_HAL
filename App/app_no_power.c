#include "app_no_power.h"
#include "bsp_adc_safe.h"
#include "bsp_encoder_safe.h"
#include "bsp_motor_enable.h"
#include "bsp_safety.h"
#include "bsp_timer.h"
#include "tim.h"
#include "bsp_timing.h"
#include <math.h>

#ifndef DMF407_NO_POWER_CONTROL
#define DMF407_NO_POWER_CONTROL 0
#endif
/* Diagnostic timeout only, NOT a current-loop sampling budget. Poll in main
 * with SysTick running; at most six finite waits, no ADC polling in an ISR. */
#define ADC_POLL_TIMEOUT_MS 2U
#define SAMPLE_MAX_AGE_MS 20U
#define ENCODER_MAX_INTERVAL_MS 2U
static FOC_Pipeline pipeline;
static volatile FOC_RawSample published;
static volatile uint32_t sample_time;
static volatile int sample_valid, angle_valid;
static uint32_t encoder_time;
static int running;
static App_MotionSim motion_sim;

static void angle_tick(void)
{
    if (BSP_Safety_GetFault()!=BSP_FAULT_NONE) { return; }
    uint32_t now=HAL_GetTick();
    if ((uint32_t)(now-encoder_time)>ENCODER_MAX_INTERVAL_MS) {
        BSP_Safety_Fault(BSP_FAULT_ENCODER); return;
    }
    uint16_t raw;
    int64_t total;
    if (!BSP_EncoderSafe_Sample(&raw,&total)) { BSP_Safety_Fault(BSP_FAULT_ENCODER); return; }
#if DMF407_NO_POWER_CONTROL
    Motor_AngleSpeedResult angle;
    /* V2 bounded phase avoids int32 overflow/large float angle precision loss.
     * Encoder resolution, offset, pole pairs and nominal 1 ms stay unchanged. */
    Motor_AngleSpeedCalculate(&pipeline.angle_state,raw,(int32_t)(total%4000),&angle);
    uint32_t mask=__get_PRIMASK();
    __disable_irq();
    pipeline.angle=angle;
    encoder_time=now;
    angle_valid=1;
    __set_PRIMASK(mask);
#else
    (void)raw; (void)total;
    encoder_time=now;
#endif
}

#if DMF407_NO_POWER_CONTROL
static void control_tick(void)
{
    if (BSP_Safety_GetFault()!=BSP_FAULT_NONE || !sample_valid || !angle_valid) { return; }
    if ((uint32_t)(HAL_GetTick()-sample_time)>SAMPLE_MAX_AGE_MS ||
        (uint32_t)(HAL_GetTick()-encoder_time)>ENCODER_MAX_INTERVAL_MS) {
        BSP_Safety_Fault(BSP_FAULT_STALE); return;
    }
    FOC_RawSample raw={ published.bus,published.v,published.w };
    /* Main publishes with IRQs briefly masked; TIM1 preempts TIM2 only before
     * or after its angle commit. No ADC waits or full math under PRIMASK. */
    if (raw.bus>4095 || raw.v>4095 || raw.w>4095 ||
        !isfinite(pipeline.angle.electrical_angle) || !isfinite(pipeline.angle.speed_rpm) ||
        !isfinite(pipeline.speed_ref) || !isfinite(pipeline.id_ref) || !isfinite(pipeline.iq_ref)) {
        BSP_Safety_Fault(BSP_FAULT_CONTROL); return;
    }
    FOC_TickResult result=FOC_PipelineControlTick(&pipeline,&raw,1);
    if (result!=FOC_TICK_VALID) { BSP_Safety_Fault(BSP_FAULT_CONTROL); return; }
    (void)BSP_PWM_ApplyChecked(&pipeline.pwm);
}
#endif

int App_NoPower_Start(void)
{
    BSP_MotorEnable_ForceDisable();
    /* No restart after run/fault. Only a MCU reset recreates PI/raw state. */
    if (running || BSP_Safety_GetFault()!=BSP_FAULT_NONE) { return 0; }
    if (!BSP_TIM1_ConfigChecked()) { return 0; }
    Motion_ProfileConfig motion_config=Motion_ProfileLegacyConfig();
    if (App_MotionSim_Init(&motion_sim,&motion_config)!=MOTION_OK ||
        App_MotionSim_LoadLegacyDemo(&motion_sim)!=MOTION_OK) {
        BSP_Safety_Fault(BSP_FAULT_CONTROL); return 0;
    }
    BSP_Timing_Init();
    FOC_PipelineInit(&pipeline);
    sample_valid=angle_valid=0;
    if (!BSP_ADC_StartChecked()) { return 0; }
    /* V2 moves encoder start before TIM2, so its init-generated update cannot
     * sample an uninitialized domain. Preserve TIM2/TIM4 UIF policy. */
    if (!BSP_EncoderSafe_Start()) { BSP_Safety_Fault(BSP_FAULT_ENCODER); return 0; }
#if DMF407_NO_POWER_CONTROL
    if (!BSP_PWM_StartChecked()) { return 0; }
#endif
    encoder_time=HAL_GetTick();
#if DMF407_NO_POWER_CONTROL
    BSP_Timer_SetHooks(control_tick,angle_tick,BSP_EncoderSafe_OverflowTick);
#else
    BSP_Timer_SetHooks(0,angle_tick,BSP_EncoderSafe_OverflowTick);
#endif
    if (!BSP_Timer_StartChecked()) { return 0; }
    if (BSP_Safety_GetFault()!=BSP_FAULT_NONE) { return 0; }
#if DMF407_NO_POWER_CONTROL
    if (!BSP_Timer_StartTim1Diagnostic()) { return 0; }
#else
    if (!BSP_TIM1_StartDiagnosticChecked()) { return 0; }
#endif
    running=1;
    return 1;
}

void App_NoPower_Poll(void)
{
    if (BSP_Safety_GetFault()!=BSP_FAULT_NONE) { Motion_Fault(&motion_sim.motion); return; }
    if (!running) { return; }
    FOC_RawSample sample;
    if (!BSP_ADC_ReadAllChecked(&sample,ADC_POLL_TIMEOUT_MS)) {
        Motion_Fault(&motion_sim.motion); return;
    }
    uint32_t mask=__get_PRIMASK();
    __disable_irq();
    if (BSP_Safety_GetFault()==BSP_FAULT_NONE) {
        published=sample;
        sample_time=HAL_GetTick();
        __DMB();
        sample_valid=1;
    }
    __set_PRIMASK(mask);
    /* Pure logical sink: one accepted XY event per successful main poll.
     * Never write TIM4 ARR/GPIO or use this as a timed/physical step claim. */
    if (BSP_Safety_GetFault()==BSP_FAULT_NONE) { (void)App_MotionSim_Step(&motion_sim); }
    else { Motion_Fault(&motion_sim.motion); }
}

void App_NoPower_GetMotionSnapshot(App_MotionSnapshot *out)
{
    if (BSP_Safety_GetFault()!=BSP_FAULT_NONE) { Motion_Fault(&motion_sim.motion); }
    App_MotionSim_Read(&motion_sim,out);
}

void App_NoPower_CancelMotion(void) { Motion_Cancel(&motion_sim.motion); }

void App_NoPower_GetSnapshot(FOC_Pipeline *out)
{
    /* Debug/readback from main only. Not used by the realtime ISR. */
    uint32_t mask=__get_PRIMASK();
    __disable_irq();
    *out=pipeline;
    __set_PRIMASK(mask);
}
