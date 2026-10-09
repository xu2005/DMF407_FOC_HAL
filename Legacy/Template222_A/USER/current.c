#include <stdio.h>
#include "current.h"
#include "stm32f4xx.h"
#include "drv_config.h"  // 包含您的配置头文件
#include <math.h> // 包含数学库以使用cos和sin函数
#include "globals.h"
#include "adc.h"



//#define VOLTAGE_SENSOR_SCALE 1.0 // 根据实际情况调整
#define CURRENT_SENSOR_SCALE 0.12 // 根据你的公式调整
// static uint8_t channel_index = 0;  // 用于跟踪当前正在读取的通道索引

float calculate_bus_voltage(uint16_t adc_value) 
{
    float V_BUS = (adc_value / (float)ADC_RESOLUTION) * ADC_REF_VOLTAGE;
    return V_BUS * 25;
}

float calculate_current(uint16_t adc_value) 
{
    float V_adc = (adc_value / (float)ADC_RESOLUTION) * ADC_REF_VOLTAGE;
    return (V_adc - 1.250) / CURRENT_SENSOR_SCALE;
}

void process_ADC_data(void)
{
    // 获取注入模式采集到的ADC值
    
            // 采样w相和v相，推导u相电流
	
 uint16_t    bus_v=adc1_value;
 uint16_t    adc2_v_curr=adc2_value; 
 uint16_t    adc3_v_curr= adc3_value;

            w_current = calculate_current(adc3_v_curr); //
            v_current = calculate_current(adc2_v_curr); 
            u_current = -w_current - v_current;       
          bus_voltage=calculate_bus_voltage(bus_v);
	
}

void ADC_bus(void)
{
      float sum_bus = 0.0f;
    int num_samples = 20;

    for (int i = 0; i < num_samples; i++) 
    {
       
        sum_bus += bus_voltage; // 采样母线电压
    }
    
    // 计算平均值
  bus= sum_bus / num_samples;
		
	}




