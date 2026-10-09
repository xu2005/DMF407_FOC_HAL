#include "angle.h"
#include "stm32f4xx.h"
#include <math.h>
#include "encoderini.h"
#include "globals.h"
#include "drv_config.h"  // 包含您的配置头文件




float mechanical_angle_offset=0.546637118;  //0.378561974
//float mechanical_an=0.0f;
volatile int32_t Previous_Count = 0;  // 用于记录上次的编码器计数值

void TIM3_IRQHandler(void)
{
   
	
      if (TIM_GetITStatus(TIM3, TIM_IT_Update) != RESET) // 检查更新中断 // 检查更新中断
    {
        TIM_ClearITPendingBit(TIM3, TIM_IT_Update);  // 清除中断标志位
        
        if (TIM3->CR1 & TIM_CR1_DIR)  // 判断是否为向下计数
        {
            g_timx_encode_count--;  // 如果向下计数，减少溢出次数
        }
        else  // 向上计数
        {
            g_timx_encode_count++;  // 增加溢出次数
        }
    }
}
    
int32_t gtim_get_encode(void)
{
    // 函数实现
    return (int32_t)(TIM_GetCounter(TIM3) + g_timx_encode_count * 65536);
}
void calculate_motor_parameters(float* electrical_angle, float* motor_speed)
{
    // 获取编码器的总计数值
	  long Count; 
  	u32 Current_Count=TIM_GetCounter(TIM3);
   	int Enc_Timer_Overflow_one;
    int32_t total_count = gtim_get_encode();  // 从编码器获取当前计数值
	  Enc_Timer_Overflow_one=g_timx_encode_count;
	 Enc_Timer_Overflow_one=0;
	
	/*
	if(TIM3->CR1 & TIM_CR1_DIR)
	{
		
		Count = -(Current_Count - Previous_Count);   //计算出一个时间转过的脉冲数
	}
	else
	{		//如果正转
		Count = (Current_Count - Previous_Count);       //计算出一个时间转过的脉冲数
	}
	*/
	Count = (Current_Count - Previous_Count);       //计算出一个时间转过的脉冲数
	
if (Count < -32768) 
	{
    Count = Count + 65536; // 65536 是 2^16
} else if (Count > 32767) 
{
    Count -=65536;
}
     Previous_Count = Current_Count; 
		

    // 时间间隔 delta_time 假设为 0.01 秒 (10 毫秒)
    float delta_time = 0.001f;


    // 计算机械角度，基于总脉冲数
   float mechanical_angle = ((float)total_count / (float)ENCODER_RESOLUTION) * 2.0f * PI;

    // 通过 fmod 确保机械角度在 0 到 2π 之间
    mechanical_angle = fmod(mechanical_angle, 2.0f * PI);
    if (mechanical_angle < 0) {
        mechanical_angle += 2.0f * PI;
    }
    
    // 考虑机械角度的零点偏移
    mechanical_angle = mechanical_angle - mechanical_angle_offset;
    if (mechanical_angle < 0) {
        mechanical_angle += 2.0f * PI;
    }

    // 计算电角度
    *electrical_angle = mechanical_angle * POLE_PAIRS;
    *electrical_angle = fmod(*electrical_angle, 2.0f * PI);
    if (*electrical_angle < 0) {
        *electrical_angle += 2.0f * PI;
    }
//theta=*electrical_angle;
    // 假设电机速度的计算基于上一个时间间隔的脉冲数 delta_count

    // 计算电机速度 (RPM)
    *motor_speed = (float)Count / (float)(ENCODER_RESOLUTION) * 60.0f / delta_time;
//		Speed_actual= *motor_speed;
}

//float calculate_motor_speed(void)
//{
//    // 获取当前编码器计数值
//    int32_t current_encoder_count = gtim_get_encode();

//    // 计算计数变化量
//    int32_t delta_count = current_encoder_count - last_encoder_count;

//    // 更新上次计数值
//    last_encoder_count = current_encoder_count;

//    // 计算转速，单位：RPM
//    float speed_rpm = ((float)delta_count / ENCODER_RESOLUTION) * (60.0f / TIME_INTERVAL);

////    return speed_rpm;
//}
