#ifndef MOTION_PROFILE_H
#define MOTION_PROFILE_H
#include <stdint.h>

#define MOTION_ARRAY_LENGTH 3000U
typedef struct {
    int32_t show_speed, position_limit;
    double step_degrees, vmin_rpm, duration_ms;
    uint32_t subdivision;
    float flexibility;
} Motion_ProfileConfig;
typedef struct {
    uint16_t period[MOTION_ARRAY_LENGTH + 1U];
    uint32_t dec_len; /* samples before terminal period-1 entry */
    uint16_t period_min, period_max;
    int valid;
} Motion_Profile;
typedef enum {
    MOTION_PROFILE_OK = 0, MOTION_PROFILE_INPUT, MOTION_PROFILE_CAPACITY
} Motion_ProfileResult;

Motion_ProfileConfig Motion_ProfileLegacyConfig(void);
/* Matches cul_Saccdec's float intermediates, clamps and terminal entry.
 * Caller owns storage. Failure invalidates metadata; never consume partial data.
 * No HAL, timer writes or physical frequency guarantee. */
Motion_ProfileResult Motion_ProfileBuild(Motion_Profile *out, const Motion_ProfileConfig *config);
#endif
