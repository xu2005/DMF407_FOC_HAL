#ifndef BSP_TIMING_H
#define BSP_TIMING_H
#include <stdint.h>
#ifndef DMF407_DWT_DIAGNOSTICS
#define DMF407_DWT_DIAGNOSTICS 0
#endif
typedef struct { uint32_t enabled, count, last_cycles, max_cycles; } BSP_TimingSnapshot;
#if DMF407_DWT_DIAGNOSTICS
#include "main.h"
void BSP_Timing_Init(void);
void BSP_Timing_RecordTim1(uint32_t start);
void BSP_Timing_GetSnapshot(BSP_TimingSnapshot *out);
static inline uint32_t BSP_Timing_Begin(void) { return DWT->CYCCNT; }
#else
static inline void BSP_Timing_Init(void) { }
static inline uint32_t BSP_Timing_Begin(void) { return 0; }
static inline void BSP_Timing_RecordTim1(uint32_t start) { (void)start; }
static inline void BSP_Timing_GetSnapshot(BSP_TimingSnapshot *out)
{ if (out) { *out = (BSP_TimingSnapshot){0,0,0,0}; } }
#endif
#endif
