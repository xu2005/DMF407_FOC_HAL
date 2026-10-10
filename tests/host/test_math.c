#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "motor_angle_speed.h"
#include "foc_transforms.h"
#include "math_angle_vectors.h"

#define CHECK(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #condition); \
        exit(EXIT_FAILURE); \
    } \
} while (0)

static void near_value(float actual, float expected, float tolerance)
{
    if (!(fabsf(actual - expected) <= tolerance)) {
        fprintf(stderr, "FAIL actual=%.9g expected=%.9g tolerance=%.9g\n",
                actual, expected, tolerance);
        exit(EXIT_FAILURE);
    }
}

static void expect_angle(const Motor_AngleSpeedResult *result,
                         const struct angle_vector *vector)
{
    near_value(result->mechanical_angle, vector->mechanical, 2e-6f);
    near_value(result->electrical_angle, vector->electrical, 2e-6f);
    near_value(result->speed_rpm, vector->rpm, 1e-3f);
}

static void test_angle(void)
{
    Motor_AngleSpeedState state = { 0 };
    Motor_AngleSpeedResult result;
    for (unsigned i = 0; i < 10; ++i) {
        const struct angle_vector *vector = &angle_vectors[i];
        /* sample_2 must inherit the actual state written by sample_1. */
        if (i != 4) state.previous_count = vector->previous;
        CHECK(state.previous_count == vector->previous);
        Motor_AngleSpeedCalculate(&state, vector->raw, vector->total, &result);
        expect_angle(&result, vector);
        CHECK(state.previous_count == (int32_t)vector->raw);
        printf("PASS angle %-20s mech=%.9g elec=%.9g rpm=%.9g\n",
               vector->name, result.mechanical_angle, result.electrical_angle, result.speed_rpm);
    }
}

static void test_angle_r01(void)
{
    const unsigned nominal_indexes[] = { 5, 6 };
    const unsigned faulty_indexes[] = { 10, 11 };
    for (unsigned i = 0; i < 2; ++i) {
        const struct angle_vector *nominal = &angle_vectors[nominal_indexes[i]];
        const struct angle_vector *faulty = &angle_vectors[faulty_indexes[i]];
        Motor_AngleSpeedState good_state = { nominal->previous };
        Motor_AngleSpeedState bad_state = { faulty->previous };
        Motor_AngleSpeedResult good, bad;
        Motor_AngleSpeedCalculate(&good_state, nominal->raw, nominal->total, &good);
        Motor_AngleSpeedCalculate(&bad_state, faulty->raw, faulty->total, &bad);
        expect_angle(&good, nominal);
        expect_angle(&bad, faulty);
        CHECK(faulty->raw == nominal->raw);
        CHECK(faulty->total - nominal->total == (i == 0 ? -65536 : 65536));
        CHECK(fabsf(bad.mechanical_angle - good.mechanical_angle) > 1.0f);
        CHECK(fabsf(bad.electrical_angle - good.electrical_angle) > 1.0f);
        near_value(bad.speed_rpm, good.speed_rpm, 1e-3f);
        printf("KNOWN RISK R-01 total error=%d: mech %.9g->%.9g, elec %.9g->%.9g; raw-based rpm stays %.9g\n",
               faulty->total - nominal->total, good.mechanical_angle, bad.mechanical_angle,
               good.electrical_angle, bad.electrical_angle, bad.speed_rpm);
    }
    puts("KNOWN RISK propagation reproduced; no encoder fix or risk closure");
}

static void test_transforms(void)
{
    /* Independent fixed references from SYSTEM/ipark.c expressions;
     * no chaining a tested transform into another's expected value. */
    static const float clarke_cases[][4] = {
        { 0, 0, 0, 0 },
        { 3, -1, 3, 0.5773502692f },
        { -2, -1, -2, -2.309401077f }
    };
    for (unsigned i = 0; i < 3; ++i) {
        FOC_AlphaBetaCurrents result;
        FOC_Clarke(clarke_cases[i][0], clarke_cases[i][1], &result);
        near_value(result.I_alpha, clarke_cases[i][2], 1e-6f);
        near_value(result.I_beta, clarke_cases[i][3], 1e-6f);
    }
    /* alpha, beta, sin, cos, q, d */
    static const float park_cases[][6] = {
        { 0, 0, 0.6f, 0.8f, 0, 0 },
        { 1, 2, 0, 1, 2, 1 },
        { 3, -2, 0.6f, 0.8f, -3.4f, 1.2f },
        { -3, 2, 0.6f, 0.8f, 3.4f, -1.2f }
    };
    for (unsigned i = 0; i < 4; ++i) {
        FOC_AlphaBetaCurrents input = { park_cases[i][0], park_cases[i][1] };
        FOC_DQCurrents result;
        FOC_Park(&input, park_cases[i][2], park_cases[i][3], &result);
        near_value(result.I_q, park_cases[i][4], 1e-6f);
        near_value(result.I_d, park_cases[i][5], 1e-6f);
    }
    /* d, q, sin, cos, alpha, beta */
    static const float inverse_cases[][6] = {
        { 0, 0, 0.6f, 0.8f, 0, 0 },
        { 1.2f, -3.4f, 0.6f, 0.8f, 3, -2 },
        { -1.2f, 3.4f, 0.6f, 0.8f, -3, 2 },
        { 1, 2, 1, 0, -2, 1 }
    };
    for (unsigned i = 0; i < 4; ++i) {
        FOC_DQVoltage input = { inverse_cases[i][0], inverse_cases[i][1] };
        FOC_AlphaBetaVoltage result;
        FOC_InversePark(&input, &result, inverse_cases[i][2], inverse_cases[i][3]);
        near_value(result.alpha, inverse_cases[i][4], 1e-6f);
        near_value(result.beta, inverse_cases[i][5], 1e-6f);
    }
    puts("PASS transforms: Clarke 3 / Park 4 / Inverse Park 4 fixed vectors; pure arithmetic only");
}

int main(int argc, char **argv)
{
    CHECK(argc == 2);
    if (strcmp(argv[1], "angle") == 0) test_angle();
    else if (strcmp(argv[1], "angle_r01") == 0) test_angle_r01();
    else if (strcmp(argv[1], "transforms") == 0) test_transforms();
    else { fprintf(stderr, "Unknown math test group: %s\n", argv[1]); return EXIT_FAILURE; }
    return EXIT_SUCCESS;
}
