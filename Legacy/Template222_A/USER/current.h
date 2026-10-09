#ifndef __CURRENT_H
#define __CURRENT_H

#include "stm32f4xx.h"

#include "current.h"

float calculate_bus_voltage(uint16_t adc_value); 

float calculate_current(uint16_t adc_value);
void process_ADC_data(void);
void ADC_bus(void);

// º¯ÊýÉùÃ÷
#endif
