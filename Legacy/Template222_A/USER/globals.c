#include "globals.h"
#include <stdint.h>  // ȷ���������ͷ�ļ�
#include <math.h> // ������ѧ����ʹ��cos��sin����

 volatile float theta = 0.0f;  
int g_timx_encode_count = 0;

 uint16_t adc1_value=0;
 uint16_t adc2_value=0; 
 uint16_t adc3_value=0;

volatile float u_current=0.0f;
volatile float v_current=0.0f;
volatile float w_current=0.0f;
float bus_voltage=0.0f;
float bus=0.0f;
float id_Ref= 0; 
float iq_Ref=0;
float id_Kp= 0.5;    //0.009f //0.09f
float id_Ki=0.05;
float iq_Kp=1;
float iq_Ki= 0.4;	
float Speed_Ref= 100.0f; // 启动调试：目标约100rpm
float Speed_Kp= 0.08f; // 适度提高启动响应
float Speed_Ki= 0.0015f; // 适度提高积分以克服静摩擦	
float current_Umax=18;
float current_Umin=-18;
float Speed_Imax=18;
float Speed_Imin=-18;

long Position_Limit=1000;
long Position_Ref_X=0,Position_Ref_Y=0;
float Position_Kp= 10.0f;
float Position_Kff=5.0f;
long Position_actual=0;
//DQ_Def dq = {0.0f, 0.0f};  // ���岢��ʼ�� dq_global ����
float w_current_base=0.0f;
float  v_current_base=0.0f;
   float sum_d = 0.0f;
    float sum_q = 0.0f;
		float sum_speed=0;
int enable_flag = 1; // 上电即使能，便于启动验证
float Speed_actual=0;

 






