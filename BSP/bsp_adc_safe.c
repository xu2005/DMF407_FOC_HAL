#include "bsp_adc_safe.h"
#include "bsp_safety.h"
#include "adc.h"
static int started;
static void stop_all(void)
{
    /* Shutdown gate is independent of ADC stop success. */
    (void)HAL_ADC_Stop(&hadc1);
    (void)HAL_ADC_Stop(&hadc2);
    (void)HAL_ADC_Stop(&hadc3);
    started=0;
}
int BSP_ADC_StartChecked(void)
{
    if (BSP_Safety_GetFault()!=BSP_FAULT_NONE) { return 0; }
    if (HAL_ADC_Start(&hadc1)!=HAL_OK || HAL_ADC_Start(&hadc2)!=HAL_OK ||
        HAL_ADC_Start(&hadc3)!=HAL_OK) {
        BSP_Safety_Fault(BSP_FAULT_ADC_START);
        stop_all();
        return 0;
    }
    started=1;
    return 1;
}
int BSP_ADC_ReadAllChecked(FOC_RawSample *sample, uint32_t timeout_ms)
{
    if (BSP_Safety_GetFault()!=BSP_FAULT_NONE) { return 0; }
    if (!started || !sample || timeout_ms==HAL_MAX_DELAY) {
        BSP_Safety_Fault(BSP_FAULT_ADC_READ);
        stop_all();
        return 0;
    }
    ADC_HandleTypeDef *order[] = { &hadc3,&hadc2,&hadc1,&hadc3,&hadc2,&hadc1 };
    uint16_t values[6];
    for (unsigned i=0; i<6; ++i) {
        if (HAL_ADC_PollForConversion(order[i],timeout_ms)!=HAL_OK) {
            BSP_Safety_Fault(BSP_FAULT_ADC_READ);
            stop_all();
            return 0;
        }
        uint32_t value=HAL_ADC_GetValue(order[i]);
        if (value>4095U) {
            BSP_Safety_Fault(BSP_FAULT_ADC_READ);
            stop_all();
            return 0;
        }
        values[i]=(uint16_t)value;
    }
    if (BSP_Safety_GetFault()!=BSP_FAULT_NONE) { return 0; }
    *sample=(FOC_RawSample){ values[5],values[4],values[3] };
    return 1;
}
