#ifndef MOCK_HAL_H
#define MOCK_HAL_H

#include <stdint.h>

/* Minimal HAL boundary, not a peripheral timing model. */
typedef enum { HAL_OK = 0, HAL_ERROR, HAL_BUSY, HAL_TIMEOUT } HAL_StatusTypeDef;
typedef struct {
    uint32_t CR1, DIER, SR, CNT, ARR, CCR1, CCR2, CCR3, BDTR, CCER;
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
#define TIM_BDTR_MOE 0x8000U
#define TIM_BDTR_AOE 0x4000U
#define TIM_CCER_CC1E 0x1U
#define TIM_CCER_CC1NE 0x4U
#define TIM_CCER_CC2E 0x10U
#define TIM_CCER_CC2NE 0x40U
#define TIM_CCER_CC3E 0x100U
#define TIM_CCER_CC3NE 0x400U
extern uint32_t mock_primask;
#define __get_PRIMASK() (mock_primask)
#define __disable_irq() (mock_primask = 1U)
#define __set_PRIMASK(v) (mock_primask = (v))
#define __DMB() ((void)0)

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
#define __HAL_TIM_DISABLE_IT(h, i) ((h)->Instance->DIER &= ~(i))

uint32_t *mock_compare_register(TIM_HandleTypeDef *handle, uint32_t channel);
#define __HAL_TIM_SET_COMPARE(h, c, v) (*mock_compare_register((h), (c)) = (v))
#define __HAL_TIM_GET_COMPARE(h, c) (*mock_compare_register((h), (c)))

HAL_StatusTypeDef HAL_TIM_Encoder_Start(TIM_HandleTypeDef *handle, uint32_t channel);
HAL_StatusTypeDef HAL_TIM_Encoder_Stop(TIM_HandleTypeDef *handle, uint32_t channel);
HAL_StatusTypeDef HAL_TIM_Base_Start_IT(TIM_HandleTypeDef *handle);
HAL_StatusTypeDef HAL_TIM_Base_Stop_IT(TIM_HandleTypeDef *handle);
HAL_StatusTypeDef HAL_ADC_Start(ADC_HandleTypeDef *handle);
HAL_StatusTypeDef HAL_ADC_Stop(ADC_HandleTypeDef *handle);
uint32_t HAL_GetTick(void);
HAL_StatusTypeDef HAL_ADC_PollForConversion(ADC_HandleTypeDef *handle, uint32_t timeout);
uint32_t HAL_ADC_GetValue(ADC_HandleTypeDef *handle);
HAL_StatusTypeDef HAL_TIM_PWM_Start(TIM_HandleTypeDef *handle, uint32_t channel);
HAL_StatusTypeDef HAL_TIMEx_PWMN_Start(TIM_HandleTypeDef *handle, uint32_t channel);
HAL_StatusTypeDef HAL_TIM_PWM_Stop(TIM_HandleTypeDef *handle, uint32_t channel);
HAL_StatusTypeDef HAL_TIMEx_PWMN_Stop(TIM_HandleTypeDef *handle, uint32_t channel);

#endif
