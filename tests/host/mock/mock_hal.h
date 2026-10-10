#ifndef MOCK_HAL_H
#define MOCK_HAL_H

#include <stdint.h>

/* Minimal HAL boundary, not a peripheral timing model. */
typedef enum { HAL_OK = 0, HAL_ERROR, HAL_BUSY, HAL_TIMEOUT } HAL_StatusTypeDef;
typedef struct {
    uint32_t CR1, DIER, SR, CNT, ARR, CCR1, CCR2, CCR3;
} TIM_TypeDef;
typedef struct { TIM_TypeDef *Instance; } TIM_HandleTypeDef;
typedef struct { unsigned id; } ADC_HandleTypeDef;

#define TIM_CR1_CEN 0x0001U
#define TIM_CR1_DIR 0x0010U
#define TIM_IT_UPDATE 0x0001U
#define TIM_IT_CC1 0x0002U
#define TIM_IT_CC2 0x0004U
#define TIM_FLAG_UPDATE 0x0001U
#define TIM_CHANNEL_1 0x0000U
#define TIM_CHANNEL_2 0x0004U
#define TIM_CHANNEL_3 0x0008U
#define TIM_CHANNEL_ALL 0x003CU
#define HAL_MAX_DELAY UINT32_MAX

enum mock_event_kind {
    COUNTER_SET, ENCODER_START, FLAG_CLEAR, IT_ENABLE,
    ADC_START, ADC_POLL, ADC_VALUE,
    PWM_START, PWMN_START, PWM_STOP, PWMN_STOP
};
void mock_record(enum mock_event_kind kind, const void *handle, uint32_t value);

#define __HAL_TIM_SET_COUNTER(h, v) \
    (mock_record(COUNTER_SET, (h), (v)), (h)->Instance->CNT = (v))
#define __HAL_TIM_GET_COUNTER(h) ((h)->Instance->CNT)
#define __HAL_TIM_IS_TIM_COUNTING_DOWN(h) ((h)->Instance->CR1 & TIM_CR1_DIR)
/* Model the effect of write-zero-to-clear, not the bus write encoding. */
#define __HAL_TIM_CLEAR_FLAG(h, f) \
    (mock_record(FLAG_CLEAR, (h), (f)), (h)->Instance->SR &= ~(f))
#define __HAL_TIM_ENABLE_IT(h, i) \
    (mock_record(IT_ENABLE, (h), (i)), (h)->Instance->DIER |= (i))

uint32_t *mock_compare_register(TIM_HandleTypeDef *handle, uint32_t channel);
#define __HAL_TIM_SET_COMPARE(h, c, v) (*mock_compare_register((h), (c)) = (v))
#define __HAL_TIM_GET_COMPARE(h, c) (*mock_compare_register((h), (c)))

HAL_StatusTypeDef HAL_TIM_Encoder_Start(TIM_HandleTypeDef *handle, uint32_t channel);
HAL_StatusTypeDef HAL_ADC_Start(ADC_HandleTypeDef *handle);
HAL_StatusTypeDef HAL_ADC_PollForConversion(ADC_HandleTypeDef *handle, uint32_t timeout);
uint32_t HAL_ADC_GetValue(ADC_HandleTypeDef *handle);
HAL_StatusTypeDef HAL_TIM_PWM_Start(TIM_HandleTypeDef *handle, uint32_t channel);
HAL_StatusTypeDef HAL_TIMEx_PWMN_Start(TIM_HandleTypeDef *handle, uint32_t channel);
HAL_StatusTypeDef HAL_TIM_PWM_Stop(TIM_HandleTypeDef *handle, uint32_t channel);
HAL_StatusTypeDef HAL_TIMEx_PWMN_Stop(TIM_HandleTypeDef *handle, uint32_t channel);

#endif
