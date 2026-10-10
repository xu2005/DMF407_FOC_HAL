#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tim.h"
#include "adc.h"
#include "bsp_encoder.h"
#include "bsp_adc.h"
#include "bsp_pwm.h"

/* Explicit checks stay enabled in Release/NDEBUG builds. */
#define CHECK(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #condition); \
        exit(EXIT_FAILURE); \
    } \
} while (0)

static TIM_TypeDef tim1_regs, tim3_regs;
TIM_HandleTypeDef htim1 = { &tim1_regs }, htim3 = { &tim3_regs };
ADC_HandleTypeDef hadc1 = { 1 }, hadc2 = { 2 }, hadc3 = { 3 };
static struct {
    enum mock_event_kind kind;
    const void *handle;
    uint32_t value;
} events[32];
static unsigned event_count;
static unsigned adc_cursor[3];
static const uint16_t adc_samples[3][2] = {
    { 101, 4095 }, /* BUS: first discarded, second retained */
    { 202, 0 },    /* V */
    { 303, 2048 }  /* W */
};

void mock_record(enum mock_event_kind kind, const void *handle, uint32_t value)
{
    CHECK(event_count < sizeof(events) / sizeof(events[0]));
    events[event_count].kind = kind;
    events[event_count].handle = handle;
    events[event_count++].value = value;
}

static void expect_event(unsigned index, enum mock_event_kind kind,
                         const void *handle, uint32_t value)
{
    CHECK(index < event_count);
    CHECK(events[index].kind == kind);
    CHECK(events[index].handle == handle);
    CHECK(events[index].value == value);
}

uint32_t *mock_compare_register(TIM_HandleTypeDef *handle, uint32_t channel)
{
    switch (channel) {
    case TIM_CHANNEL_1: return &handle->Instance->CCR1;
    case TIM_CHANNEL_2: return &handle->Instance->CCR2;
    case TIM_CHANNEL_3: return &handle->Instance->CCR3;
    default: CHECK(0); return NULL;
    }
}

HAL_StatusTypeDef HAL_TIM_Encoder_Start(TIM_HandleTypeDef *handle, uint32_t channel)
{
    mock_record(ENCODER_START, handle, channel);
    handle->Instance->CR1 |= TIM_CR1_CEN;
    return HAL_OK; /* No implicit UIE/CC1IE/CC2IE. */
}

HAL_StatusTypeDef HAL_ADC_Start(ADC_HandleTypeDef *handle)
{
    mock_record(ADC_START, handle, 0);
    return HAL_OK;
}

HAL_StatusTypeDef HAL_ADC_PollForConversion(ADC_HandleTypeDef *handle, uint32_t timeout)
{
    mock_record(ADC_POLL, handle, timeout);
    return HAL_OK; /* EOC already available; no simulated blocking time. */
}

uint32_t HAL_ADC_GetValue(ADC_HandleTypeDef *handle)
{
    CHECK(handle->id >= 1 && handle->id <= 3);
    unsigned index = handle->id - 1;
    CHECK(adc_cursor[index] < 2);
    uint16_t value = adc_samples[index][adc_cursor[index]++];
    mock_record(ADC_VALUE, handle, value);
    return value;
}

#define MOCK_PWM_FUNCTION(name, kind) \
HAL_StatusTypeDef name(TIM_HandleTypeDef *handle, uint32_t channel) \
{ mock_record((kind), handle, channel); return HAL_OK; }
MOCK_PWM_FUNCTION(HAL_TIM_PWM_Start, PWM_START)
MOCK_PWM_FUNCTION(HAL_TIMEx_PWMN_Start, PWMN_START)
MOCK_PWM_FUNCTION(HAL_TIM_PWM_Stop, PWM_STOP)
MOCK_PWM_FUNCTION(HAL_TIMEx_PWMN_Stop, PWMN_STOP)

static void test_encoder(void)
{
    tim3_regs.CNT = 19;
    tim3_regs.SR = TIM_FLAG_UPDATE | 0x0002U;
    BSP_Encoder_Start();
    CHECK(event_count == 4);
    expect_event(0, COUNTER_SET, &htim3, 0);
    expect_event(1, ENCODER_START, &htim3, TIM_CHANNEL_ALL);
    expect_event(2, FLAG_CLEAR, &htim3, TIM_FLAG_UPDATE);
    expect_event(3, IT_ENABLE, &htim3, TIM_IT_UPDATE);
    CHECK(tim3_regs.CNT == 0);
    CHECK(tim3_regs.CR1 & TIM_CR1_CEN);
    CHECK(tim3_regs.SR == 0x0002U);
    CHECK(tim3_regs.DIER == TIM_IT_UPDATE);
    CHECK((tim3_regs.DIER & (TIM_IT_CC1 | TIM_IT_CC2)) == 0);
    CHECK(BSP_Encoder_GetOverflowCount() == 0);

    tim3_regs.CNT = 17;
    CHECK(BSP_Encoder_GetRawCount() == 17);
    CHECK(BSP_Encoder_IsCountingDown() == 0);
    CHECK(BSP_Encoder_GetCount() == 17);
    tim3_regs.CR1 |= TIM_CR1_DIR;
    tim3_regs.CNT = 16;
    CHECK(BSP_Encoder_IsCountingDown() == 1);
    CHECK(BSP_Encoder_GetCount() == 16);

    tim3_regs.CR1 &= ~TIM_CR1_DIR;
    tim3_regs.CNT = 65535;
    CHECK(BSP_Encoder_GetCount() == 65535);
    tim3_regs.CNT = 0;
    BSP_Encoder_OverflowTick();
    CHECK(BSP_Encoder_GetOverflowCount() == 1);
    CHECK(BSP_Encoder_GetCount() == 65536);

    /* Reverse across the same boundary: upper wrap is undone. */
    tim3_regs.CR1 |= TIM_CR1_DIR;
    tim3_regs.CNT = 65535;
    BSP_Encoder_OverflowTick();
    CHECK(BSP_Encoder_GetOverflowCount() == 0);
    CHECK(BSP_Encoder_GetCount() == 65535);

    tim3_regs.CNT = 0;
    CHECK(BSP_Encoder_GetCount() == 0);
    tim3_regs.CNT = 65535;
    BSP_Encoder_OverflowTick();
    CHECK(BSP_Encoder_GetOverflowCount() == -1);
    CHECK(BSP_Encoder_GetCount() == -1);
    tim3_regs.CNT = 65534;
    CHECK(BSP_Encoder_GetCount() == -2);

    tim3_regs.CR1 &= ~TIM_CR1_DIR;
    tim3_regs.CNT = 0;
    BSP_Encoder_OverflowTick();
    CHECK(BSP_Encoder_GetOverflowCount() == 0);
    CHECK(BSP_Encoder_GetCount() == 0);
    puts("PASS encoder: startup/UIE only, forward/reverse, upper/lower wrap, direction reversal");
}

static void test_encoder_r01(void)
{
    /* Physical oracle: one increment from 65535 is 65536, before ISR. */
    tim3_regs.CNT = 65535;
    CHECK(BSP_Encoder_GetCount() == 65535);
    tim3_regs.CNT = 0;
    tim3_regs.SR = TIM_FLAG_UPDATE;
    int32_t observed = BSP_Encoder_GetCount();
    CHECK(observed - 65536 == -65536);
    printf("KNOWN RISK R-01 upper: CNT=0 ISR pending, expected=65536 observed=%" PRId32 " error=-65536\n", observed);
    BSP_Encoder_OverflowTick();
    tim3_regs.SR = 0;
    CHECK(BSP_Encoder_GetCount() == 65536);

    /* Restore the initial software count with a processed downward wrap. */
    tim3_regs.CR1 = TIM_CR1_DIR;
    tim3_regs.CNT = 65535;
    BSP_Encoder_OverflowTick();
    CHECK(BSP_Encoder_GetOverflowCount() == 0);
    tim3_regs.CNT = 0;
    CHECK(BSP_Encoder_GetCount() == 0);
    tim3_regs.CNT = 65535;
    tim3_regs.SR = TIM_FLAG_UPDATE;
    observed = BSP_Encoder_GetCount();
    CHECK(observed - (-1) == 65536);
    printf("KNOWN RISK R-01 lower: CNT=65535 ISR pending, expected=-1 observed=%" PRId32 " error=+65536\n", observed);
    BSP_Encoder_OverflowTick();
    tim3_regs.SR = 0;
    CHECK(BSP_Encoder_GetCount() == -1);
    puts("KNOWN RISK reproduced; assertions pass because the unfixed error was observed, not closed");
}

static void test_adc(void)
{
    BSP_ADC_Start();
    CHECK(event_count == 3);
    expect_event(0, ADC_START, &hadc1, 0);
    expect_event(1, ADC_START, &hadc2, 0);
    expect_event(2, ADC_START, &hadc3, 0);
    event_count = 0;

    uint16_t bus, v, w;
    BSP_ADC_ReadAll(&bus, &v, &w);
    ADC_HandleTypeDef *order[] = { &hadc3, &hadc2, &hadc1, &hadc3, &hadc2, &hadc1 };
    const uint16_t values[] = { 303, 202, 101, 2048, 0, 4095 };
    CHECK(event_count == 12);
    for (unsigned i = 0; i < 6; ++i) {
        expect_event(i * 2, ADC_POLL, order[i], HAL_MAX_DELAY);
        expect_event(i * 2 + 1, ADC_VALUE, order[i], values[i]);
    }
    CHECK(adc_cursor[0] == 2 && adc_cursor[1] == 2 && adc_cursor[2] == 2);
    CHECK(bus == 4095 && v == 0 && w == 2048);
    puts("PASS adc: start 1/2/3, W/V/BUS/W/V/BUS, poll-before-read, discard first 3, retain raw last 3");
}

static void test_pwm(void)
{
    const uint32_t channels[] = { TIM_CHANNEL_1, TIM_CHANNEL_2, TIM_CHANNEL_3 };
    BSP_PWM_Start();
    CHECK(event_count == 6);
    for (unsigned i = 0; i < 3; ++i) {
        expect_event(i * 2, PWM_START, &htim1, channels[i]);
        expect_event(i * 2 + 1, PWMN_START, &htim1, channels[i]);
    }
    event_count = 0;
    BSP_PWM_Stop();
    CHECK(event_count == 6);
    for (unsigned i = 0; i < 3; ++i) {
        expect_event(i * 2, PWM_STOP, &htim1, channels[i]);
        expect_event(i * 2 + 1, PWMN_STOP, &htim1, channels[i]);
    }

    tim1_regs.ARR = 99; /* Current configured period, not a waveform oracle. */
    const uint16_t cases[][3] = { {0, 0, 0}, {13, 47, 83}, {99, 0, 98} };
    for (unsigned i = 0; i < 3; ++i) {
        BSP_PWM_SetABC(cases[i][0], cases[i][1], cases[i][2]);
        CHECK(tim1_regs.CCR1 == cases[i][0]);
        CHECK(tim1_regs.CCR2 == cases[i][1]);
        CHECK(tim1_regs.CCR3 == cases[i][2]);
    }
    /* Separate register injection prevents matching setter/getter mistakes. */
    tim1_regs.CCR1 = 7; tim1_regs.CCR2 = 11; tim1_regs.CCR3 = 19;
    uint16_t u, v, w;
    BSP_PWM_GetABC(&u, &v, &w);
    CHECK(u == 7 && v == 11 && w == 19);
    CHECK(tim1_regs.ARR == 99);
    puts("PASS pwm: HAL start/stop call order, U/V/W to CCR1/2/3, zero/general/ARR boundary, independent readback");
}

int main(int argc, char **argv)
{
    CHECK(argc == 2);
    /* Each CTest case is a fresh process: BSP static overflow starts at zero. */
    if (strcmp(argv[1], "encoder") == 0) test_encoder();
    else if (strcmp(argv[1], "encoder_r01") == 0) test_encoder_r01();
    else if (strcmp(argv[1], "adc") == 0) test_adc();
    else if (strcmp(argv[1], "pwm") == 0) test_pwm();
    else { fprintf(stderr, "Unknown test group: %s\n", argv[1]); return EXIT_FAILURE; }
    return EXIT_SUCCESS;
}
