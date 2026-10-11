#ifndef APP_NO_POWER_H
#define APP_NO_POWER_H
#include "foc_pipeline.h"
#include "app_motion_sim.h"
int App_NoPower_Start(void);
void App_NoPower_Poll(void);
void App_NoPower_GetSnapshot(FOC_Pipeline *out);
/* Main-only logical Motion readback/cancel; no hardware step outputs. */
void App_NoPower_GetMotionSnapshot(App_MotionSnapshot *out);
void App_NoPower_CancelMotion(void);
#endif
