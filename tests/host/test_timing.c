#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bsp_timing.h"
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x); exit(1); } } while(0)
#if DMF407_DWT_DIAGNOSTICS
MockDWT mock_dwt;
MockCoreDebug mock_core_debug;
uint32_t mock_primask;
#endif
int main(int argc,char **argv)
{
    (void)argc; (void)argv;
    BSP_TimingSnapshot s;
#if DMF407_DWT_DIAGNOSTICS
    if (argc>1 && !strcmp(argv[1],"unsupported")) {
        mock_dwt.CTRL=DWT_CTRL_NOCYCCNT_Msk;BSP_Timing_Init();
        BSP_Timing_RecordTim1(0);BSP_Timing_GetSnapshot(&s);
        CHECK(!s.enabled && !s.count);return 0;
    }
    BSP_Timing_Init();CHECK(mock_core_debug.DEMCR&CoreDebug_DEMCR_TRCENA_Msk);
    mock_dwt.CYCCNT=UINT32_MAX-5;uint32_t start=BSP_Timing_Begin();
    mock_dwt.CYCCNT=14;BSP_Timing_RecordTim1(start); /* 20 cycles across wrap. */
    mock_dwt.CYCCNT=20;BSP_Timing_RecordTim1(14);
    mock_primask=1;BSP_Timing_GetSnapshot(&s);CHECK(mock_primask==1);
    CHECK(s.enabled && s.count==2 && s.last_cycles==6 && s.max_cycles==20);
#else
    BSP_Timing_Init();BSP_Timing_RecordTim1(BSP_Timing_Begin());BSP_Timing_GetSnapshot(&s);
    CHECK(!s.enabled && !s.count && !s.max_cycles);
#endif
    puts("PASS DWT bookkeeping only; actual cycles/WCET NEEDS HARDWARE");return 0;
}
