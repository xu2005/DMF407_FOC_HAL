#include "bsp_encoder_safe.h"
#include "encoder_tracker.h"
#include "tim.h"
#include "bsp_safety.h"
static Encoder_Tracker tracker;
static volatile uint32_t overflow_events;
static int started;
int BSP_EncoderSafe_Start(void)
{
    if (BSP_Safety_GetFault()!=BSP_FAULT_NONE) { return 0; }
    /* V2 restart resets the entire software/hardware domain while stopped.
     * Main calls this before scheduler startup; never restart from an ISR. */
    started = 0;
    __HAL_TIM_DISABLE_IT(&htim3, TIM_IT_UPDATE);
    if (HAL_TIM_Encoder_Stop(&htim3, TIM_CHANNEL_ALL) != HAL_OK) { return 0; }
    __HAL_TIM_SET_COUNTER(&htim3, 0);
    Encoder_TrackerReset(&tracker, 0);
    overflow_events = 0;
    __HAL_TIM_CLEAR_FLAG(&htim3, TIM_FLAG_UPDATE);
    if (HAL_TIM_Encoder_Start(&htim3, TIM_CHANNEL_ALL) != HAL_OK) { return 0; }
    __HAL_TIM_ENABLE_IT(&htim3, TIM_IT_UPDATE);
    started = 1;
    return 1;
}
int BSP_EncoderSafe_Sample(uint16_t *raw, int64_t *total)
{
    if (!started || !raw || !total) { return 0; }
    /* Atomic 16-bit hardware snapshot, not CNT+ISR state stitched together.
     * Only TIM2 owns tracker. TIM3 modifies only overflow_events. */
    uint16_t snapshot = (uint16_t)__HAL_TIM_GET_COUNTER(&htim3);
    if (!Encoder_TrackerUpdate(&tracker, snapshot, total)) { return 0; }
    *raw = snapshot;
    return 1;
}
void BSP_EncoderSafe_OverflowTick(void) { ++overflow_events; }
uint32_t BSP_EncoderSafe_GetOverflowEvents(void) { return overflow_events; }
