#ifndef __GLOBALS_H
#define __GLOBALS_H
#include <stdint.h>  // ȷ���������ͷ�ļ�

extern volatile float theta;
//extern volatile int pwm_update_flag; // ???,????????PWM
extern uint16_t adc1_value;
extern uint16_t adc2_value; 
extern uint16_t adc3_value;
extern int g_timx_encode_count;
extern volatile float u_current;
extern  volatile float v_current;
extern volatile float w_current;
extern float bus_voltage;
extern float bus;
extern float id_Ref; 
extern float iq_Ref;
extern float id_Kp;
extern float id_Ki;
extern float iq_Kp;
extern float iq_Ki;
extern float current_Umax;
extern float Speed_Imax;
extern float current_Umin;
extern float Speed_Imin;

extern float Speed_Ref;
extern float Speed_Kp;
extern float Speed_Ki;
extern float w_current_base;
extern float  v_current_base;
 extern int adc1_conversion_complete;
 extern int adc2_conversion_complete;
extern int adc3_conversion_complete;
    extern float sum_d;
   extern  float sum_q;
	extern int enable_flag;  // ����ʹ�ܱ�־��0Ϊ���ã�1Ϊʹ��	
extern float sum_speed;
extern float Speed_actual;

extern long Position_Limit;
extern long Position_Ref_X,Position_Ref_Y;
extern float Position_Kp;
extern float Position_Kff;
extern long Position_actual;
// ����ȫ�ֱ���
extern uint8_t current_sector;

#endif // __GLOBALS_H
