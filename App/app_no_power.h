#ifndef APP_NO_POWER_H
#define APP_NO_POWER_H
#include "foc_pipeline.h"
int App_NoPower_Start(void);
void App_NoPower_Poll(void);
void App_NoPower_GetSnapshot(FOC_Pipeline *out);
#endif
