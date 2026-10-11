#include <stdio.h>
#include <stdlib.h>
#include "tim.h"
#include "bsp_timer.h"
#include "bsp_safety.h"

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "FAIL %d: %s\n", __LINE__, #x); exit(1); } } while (0)
static TIM_TypeDef regs[5];
TIM_HandleTypeDef htim1 = { &regs[0] }, htim2 = { &regs[1] },
                  htim3 = { &regs[2] }, htim4 = { &regs[3] };
static unsigned clear_seen, enable_seen, encoder_ticks;
void mock_record(enum mock_event_kind kind, const void *handle, uint32_t value)
{
    if (handle == &htim1) {
        CHECK(value == TIM_FLAG_UPDATE);
        if (kind == FLAG_CLEAR) { CHECK(enable_seen == 0); ++clear_seen; }
        if (kind == IT_ENABLE) { CHECK(clear_seen == 1); ++enable_seen; }
    }
}
HAL_StatusTypeDef HAL_TIM_Base_Start_IT(TIM_HandleTypeDef *h)
{
    h->Instance->DIER |= TIM_IT_UPDATE;
    h->Instance->CR1 |= TIM_CR1_CEN;
    return HAL_OK;
}
void BSP_Encoder_OverflowTick(void) { ++encoder_ticks; }
void BSP_Safety_Fault(BSP_Fault fault) { (void)fault; CHECK(0); }
HAL_StatusTypeDef HAL_TIM_Base_Stop_IT(TIM_HandleTypeDef *h)
{
    h->Instance->DIER &= ~TIM_IT_UPDATE;
    h->Instance->CR1 &= ~TIM_CR1_CEN;
    return HAL_OK;
}
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *h);
int main(void)
{
    regs[0].SR = TIM_FLAG_UPDATE | 2;
    regs[0].ARR = 99;
    regs[0].CCR1 = 7;
    BSP_Timer_StartTim1Diagnostic();
    CHECK(clear_seen == 1 && enable_seen == 1);
    CHECK(regs[0].SR == 2 && regs[0].DIER == TIM_IT_UPDATE);
    CHECK(regs[0].ARR == 99 && regs[0].CCR1 == 7 && regs[0].CR1 == 0);
    regs[1].SR = regs[3].SR = TIM_FLAG_UPDATE;
    BSP_Timer_Start();
    CHECK(regs[1].SR == TIM_FLAG_UPDATE && regs[3].SR == TIM_FLAG_UPDATE);
    HAL_TIM_PeriodElapsedCallback(&htim1);
    HAL_TIM_PeriodElapsedCallback(&htim1);
    HAL_TIM_PeriodElapsedCallback(&htim2);
    HAL_TIM_PeriodElapsedCallback(&htim3);
    HAL_TIM_PeriodElapsedCallback(&htim4);
    TIM_HandleTypeDef unknown = { &regs[4] };
    HAL_TIM_PeriodElapsedCallback(&unknown);
    CHECK(BSP_Timer_GetTim1TickCount() == 2);
    CHECK(BSP_Timer_GetTim2TickCount() == 1);
    CHECK(BSP_Timer_GetTim4TickCount() == 1 && encoder_ticks == 1);
    puts("PASS diagnostic UIE routing; physical frequency/WCET not measured");
    return 0;
}
