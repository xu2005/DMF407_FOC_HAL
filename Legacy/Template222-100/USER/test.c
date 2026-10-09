#include <stdio.h>
//#include "svpwm.h"
#include "ipark.h"
#include "stm32f4xx.h"
#include "test.h"

#include "globals.h"
#include "current.h"
#include "foc_pi.h"
#include "adc.h"
#include "angle.h"
#include <math.h> // 包含数学库以使用cos和sin函数
//typedef struct {
//    int a;                // 计数器变量
//    int theta;            // 角度值 (0~360)
//    float radian_theta;   // 弧度值
//} Date_Struct;
//// 创建一个结构体实例
//Date_Struct date;
//float I_alpha=0;
//float I_beta=0;
//float I_q=0;
//float I_d=0;
	extern	float tcm1;
extern float tcm2;
extern  float tcm3;
	uint32_t current_cycle_counter = 0; // 用于计数电流环运行次数
u32 Position_feed_Count=0,Position_feed_Count_Pre=0;
long Count_feedback=0,Count_feedback_sum=0;	
extern long motorX_position,motorY_position;
extern long Position_Ref_X,Position_Ref_Y;

void test(void)
{  


//	theta=0;
   float  angle=theta;

//    // 计算 sin 和 cos 值
    float sinAn = sin(angle);
    float cosAn = cos(angle);


//    // 定义并初始化控制结构体
// 

    AlphaBeta_Def test_ab;
    SVPWM_Def svpwm_out;
    AlphaBeta_Currents ab_current_out;
    DQ_currents test_dq_currents;
	  DQ_Def test_dq;
   //
	 // test_dq.d=0;
	 //	test_dq.q=10;

    // 处理 ADC 数据
    process_ADC_data();

////    // 执行 Clarke 和 Park 变换
    clarkTransform(&ab_current_out);
    parkTransform(&ab_current_out, sinAn, cosAn, &test_dq_currents);



	/**/	
    current_cycle_counter++;
    // 每5次电流环后运行一次速度环
    if (enable_flag)
    {
		
			if (current_cycle_counter >= 10)
			{
			/*
				Position_feed_Count=TIM_GetCounter(TIM3);
				
				Count_feedback = (Position_feed_Count - Position_feed_Count_Pre);
				
				if (Count_feedback < -32768) {Count_feedback = Count_feedback + 65536;}
				else if (Count_feedback > 32767) {Count_feedback -=65536;}
				Position_feed_Count_Pre=Position_feed_Count;
				
				Position_actual+=Count_feedback;
				Position_Ref_X+=motorX_position;

				foc_pi_position(Position_Ref_X,motorX_position);				
				*/
				motorX_position=0;
				motorY_position=0;				
				
        foc_pi_speed(); // 运行速度环
        current_cycle_counter = 0; // 重置计数器
      }
	
		foc_pi_current(&test_dq_currents, &test_dq);  

    // 如果使能标志为1，执行 IPARK 和 SVPWM 运算

        IPARK(&test_dq, &test_ab, sinAn, cosAn); 
        SVPWM(&test_ab, &svpwm_out);

        // 更新 PWM 的比较值
        TIM_SetCompare1(TIM1, tcm1);
        TIM_SetCompare2(TIM1, tcm2);
        TIM_SetCompare3(TIM1, tcm3);
    }
		
		
    else
    {
        // 禁用时，将 PWM 输出设置为0
        TIM_SetCompare1(TIM1, 0);
        TIM_SetCompare2(TIM1, 0);
        TIM_SetCompare3(TIM1, 0);
    }
}


 