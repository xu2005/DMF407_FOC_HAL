#include "bsp_timing.h"
#if DMF407_DWT_DIAGNOSTICS
static volatile BSP_TimingSnapshot stats;
void BSP_Timing_Init(void)
{
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    if (DWT->CTRL & DWT_CTRL_NOCYCCNT_Msk) { return; }
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    __DSB(); __ISB();
    stats.enabled = (DWT->CTRL & DWT_CTRL_CYCCNTENA_Msk) != 0;
}
void BSP_Timing_RecordTim1(uint32_t start)
{
    uint32_t elapsed = DWT->CYCCNT - start;
    if (!stats.enabled) { return; }
    stats.last_cycles = elapsed;
    if (elapsed > stats.max_cycles) { stats.max_cycles = elapsed; }
    ++stats.count;
}
void BSP_Timing_GetSnapshot(BSP_TimingSnapshot *out)
{
    if (!out) { return; }
    uint32_t mask = __get_PRIMASK();
    __disable_irq();
    *out = stats;
    __set_PRIMASK(mask);
}
#endif
