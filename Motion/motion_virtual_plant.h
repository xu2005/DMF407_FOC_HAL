#ifndef MOTION_VIRTUAL_PLANT_H
#define MOTION_VIRTUAL_PLANT_H
#include "motion_scheduler.h"
#define MOTION_FEEDBACK_MAX_LAG 64U
typedef struct {
    uint32_t counts_per_step[2], quantum_counts[2];
    int32_t bias_counts[2];
    uint32_t feedback_lag_events; /* accepted command samples, not seconds */
    uint64_t reject_sequence, drop_sequence; /* zero disables injection */
    Motion_Axis reject_axis;
} Motion_VirtualPlantConfig;
typedef struct {
    int32_t plant_x, plant_y;
    int64_t encoder_x, encoder_y;
    double feedback_x, feedback_y;
    double planned_error_x, planned_error_y, confirmed_error_x, confirmed_error_y;
    uint64_t received, accepted, rejected, dropped, virtual_ticks;
} Motion_VirtualPlantSnapshot;
typedef struct {
    Motion_VirtualPlantConfig config;
    int32_t history[MOTION_FEEDBACK_MAX_LAG+1U][2];
    uint32_t head;
    Motion_VirtualPlantSnapshot snapshot;
} Motion_VirtualPlant;
Motion_VirtualPlantConfig Motion_VirtualPlantIdealConfig(void);
int Motion_VirtualPlantInit(Motion_VirtualPlant *plant, const Motion_VirtualPlantConfig *config,
                           int32_t x, int32_t y);
int Motion_VirtualPlantSink(void *plant, const Motion_Event *event, uint64_t ticks);
/* Quantization truncates signed encoder counts toward zero to a quantum multiple.
 * Bias remains observable: feedback = encoder / counts_per_step (steps).
 * Lag samples are held after stop; no fictitious time-based settling. */
void Motion_VirtualPlantRead(const Motion_VirtualPlant *plant,
                            const Motion_SchedulerSnapshot *scheduler,
                            Motion_VirtualPlantSnapshot *out);
#endif
