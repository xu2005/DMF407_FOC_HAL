#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "foc_pi.h"
#include "foc_svpwm.h"
#include "pi_svpwm_vectors.h"

#define COUNT(array) (sizeof(array) / sizeof((array)[0]))
static const FOC_SpeedPIParameters speed_parameters = {0.08f, 0.0015f, -18.0f, 18.0f};
static const FOC_CurrentPIParameters current_parameters = {0.5f, 0.05f, 1.0f, 0.4f, -18.0f, 18.0f};

static void check(int condition, const char *description)
{
    if (!condition) {
        fprintf(stderr, "FAIL: %s\n", description);
        exit(1);
    }
}

static void close_float(float actual, float expected, const char *description)
{
    /* A few binary32 ULPs near the largest output (50); no fast-math. */
    if (!(fabsf(actual - expected) <= 1.0e-5f)) {
        fprintf(stderr, "FAIL: %s: %.9g expected %.9g\n", description, actual, expected);
        exit(1);
    }
}

static void test_speed(void)
{
    FOC_SpeedPIState state = {0};
    for (size_t i = 0; i < COUNT(speed_vectors); ++i) {
        const SpeedVector *v = &speed_vectors[i];
        close_float(state.sum_speed, v->before, "speed state before");
        float output = FOC_SpeedPIUpdate(&state, &speed_parameters, v->ref, v->actual);
        close_float(state.sum_speed, v->after, "speed state after");
        close_float(output, v->output, "speed iq reference");
    }
    puts("PASS: speed 8 consecutive updates, integral/output clamps, zero reference retains state");
}

static void test_current(void)
{
    FOC_CurrentPIState state = {0};
    for (size_t i = 0; i < COUNT(current_vectors); ++i) {
        const CurrentVector *v = &current_vectors[i];
        FOC_DQCurrents measured = {.I_d = v->id_actual, .I_q = v->iq_actual};
        FOC_DQVoltage output;
        close_float(state.sum_d, v->before_d, "current d before");
        close_float(state.sum_q, v->before_q, "current q before");
        FOC_CurrentPIUpdate(&state, &current_parameters, v->id_ref, v->iq_ref, &measured, &output);
        close_float(state.sum_d, v->after_d, "current d after");
        close_float(state.sum_q, v->after_q, "current q after");
        close_float(output.d, v->d, "current voltage d");
        close_float(output.q, v->q, "current voltage q");
    }
    puts("PASS: current d/q 6 consecutive updates, separate integral/output clamps");
}

static void test_schedule(void)
{
    FOC_SpeedPIState speed_state = {0};
    FOC_CurrentPIState current_state = {0};
    const FOC_DQCurrents measured = {0};
    float iq_ref = 0.0f;
    unsigned speed_calls = 0, current_calls = 0;
    for (size_t i = 0; i < COUNT(schedule_vectors); ++i) {
        const ScheduleVector *v = &schedule_vectors[i];
        FOC_DQVoltage output;
        close_float(speed_state.sum_speed, v->before_speed, "schedule speed before");
        close_float(current_state.sum_d, v->before_d, "schedule d before");
        close_float(current_state.sum_q, v->before_q, "schedule q before");
        if ((i + 1) % 10 == 0) {
            iq_ref = FOC_SpeedPIUpdate(&speed_state, &speed_parameters, 100.0f, 0.0f);
            ++speed_calls;
        }
        FOC_CurrentPIUpdate(&current_state, &current_parameters, 0.0f, iq_ref, &measured, &output);
        ++current_calls;
        close_float(speed_state.sum_speed, v->after_speed, "schedule speed after");
        close_float(current_state.sum_d, v->after_d, "schedule d after");
        close_float(current_state.sum_q, v->after_q, "schedule q after");
        close_float(iq_ref, v->iq_ref, "schedule iq reference");
        close_float(output.d, v->d, "schedule voltage d");
        close_float(output.q, v->q, "schedule voltage q");
    }
    check(speed_calls == 2 && current_calls == 20, "offline 10:1 call counts");
    puts("PASS: 20 offline ticks, speed before current at ticks 10/20; no realtime scheduling claim");
}

static void test_position(void)
{
    for (size_t i = 0; i < COUNT(position_vectors); ++i) {
        const PositionVector *v = &position_vectors[i];
        float output = FOC_PositionCalculate(v->ref, v->actual, v->feed, 10.0f, 5.0f, 1000);
        close_float(output, v->output, "position pure formula");
    }
    puts("PASS: position 4 defined formula cases; Legacy global writes/diagnostics DEFERRED");
}

static void test_svpwm(void)
{
    unsigned sectors = 0;
    for (size_t i = 0; i < COUNT(svpwm_vectors); ++i) {
        const SVPWMVector *v = &svpwm_vectors[i];
        FOC_AlphaBetaVoltage input = {v->alpha, v->beta};
        FOC_SVPWMResult result;
        check(FOC_SVPWMCalculate(&input, &result) == 1, "SVPWM valid status");
        check(result.sector == v->sector, "SVPWM Legacy bit sector");
        sectors |= 1u << result.sector;
        close_float(result.ta, v->ta, "SVPWM ta");
        close_float(result.tb, v->tb, "SVPWM tb");
        close_float(result.tc, v->tc, "SVPWM tc");
        check(result.ta >= -1.0e-5f && result.ta <= 50.00001f &&
              result.tb >= -1.0e-5f && result.tb <= 50.00001f &&
              result.tc >= -1.0e-5f && result.tc <= 50.00001f,
              "SVPWM Legacy comparison-time range");
    }
    check(sectors == 0x7eu, "all six normal sectors covered");
    puts("PASS: SVPWM 14 nonzero vectors, six sectors and six overmodulation branches, two axis boundaries");
}

static void test_zero(void)
{
    const FOC_AlphaBetaVoltage input = {0.0f, 0.0f};
    FOC_SVPWMResult result;
    check(FOC_SVPWMCalculate(&input, &result) == 0, "zero has no valid three-phase output");
    check(result.sector == 0, "zero sector");
    /* Do not inspect ta/tb/tc. Do not execute Legacy's undefined-output path. */
    puts("KNOWN RISK: zero sector has no defined ta/tb/tc; new explicit status is an interface difference");
}

int main(int argc, char **argv)
{
    if (argc != 2) return 2;
    if (strcmp(argv[1], "pi_speed") == 0) test_speed();
    else if (strcmp(argv[1], "pi_current") == 0) test_current();
    else if (strcmp(argv[1], "pi_schedule") == 0) test_schedule();
    else if (strcmp(argv[1], "pi_position") == 0) test_position();
    else if (strcmp(argv[1], "svpwm_normal") == 0) test_svpwm();
    else if (strcmp(argv[1], "svpwm_zero") == 0) test_zero();
    else return 2;
    return 0;
}
