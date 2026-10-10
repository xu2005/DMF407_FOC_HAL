#ifndef FOC_PI_H
#define FOC_PI_H

#include <stdint.h>
#include "foc_transforms.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct { float sum_speed; } FOC_SpeedPIState;
typedef struct { float kp, ki, minimum, maximum; } FOC_SpeedPIParameters;
typedef struct { float sum_d, sum_q; } FOC_CurrentPIState;
typedef struct {
    float id_kp, id_ki, iq_kp, iq_ki, minimum, maximum;
} FOC_CurrentPIParameters;

/* Caller owns the original integral state (initially zero); no auto reset.
 * V1 configs: speed .08/.0015 +/-18; current d .5/.05, q 1/.4 +/-18.
 * Preserve different integral rules; there is no runtime scheduling here. */
float FOC_SpeedPIUpdate(FOC_SpeedPIState *state, const FOC_SpeedPIParameters *parameters,
                        float speed_ref, float speed_actual);
void FOC_CurrentPIUpdate(FOC_CurrentPIState *state, const FOC_CurrentPIParameters *parameters,
                         float id_ref, float iq_ref, const FOC_DQCurrents *measured,
                         FOC_DQVoltage *output);

/* Pure position formula only. position_ref_x corresponds to the Legacy global,
 * not its ignored Position_Ref argument. Valid differences fit int32_t and
 * limit is positive, as in the bounded V1 position domain. No Speed_Ref write. */
float FOC_PositionCalculate(int32_t position_ref_x, int32_t position_actual,
                            int32_t position_feed, float kp, float kff, int32_t limit);

#ifdef __cplusplus
}
#endif

#endif
