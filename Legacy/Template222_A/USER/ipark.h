#ifndef __IPARK_H
#define __IPARK_H

//#include "stm32f4xx.h"
#include "globals.h"
//// 定义 DQ_Def 结构体，包含 d 和 q 分量
typedef struct 
{
    float I_alpha;  // a?
    float I_beta;   // ???
} 
AlphaBeta_Currents;

typedef struct
{
    float I_q;  // a?
    float I_d;   // ???
} DQ_currents;
// 定义 AlphaBeta_Def 结构体，包含 alpha 和 beta 分量
typedef struct {
    float alpha;
    float beta;
} AlphaBeta_Def;

typedef struct {
    float d;
    float q;
} DQ_Def;


typedef struct {
    float Ts;
	  float U_alpha;
    float U_beta;
	float u1,u2,u3;
	int sector;
	float t1,t2,t3,t4,t5,t6,t7;
	float ta,tb,tc,X,Y,Z;
} SVPWM_Def;
//// IPARK 函数原型声明，用于执行逆Park变换
//extern DQ_Def dq;  // 声明全局变量
void clarkTransform(AlphaBeta_Currents *ab_currents);
void parkTransform(AlphaBeta_Currents *ab_currents, float sinAngle, float cosAngle, DQ_currents *dq_currents);
void clamp(float *value, float min, float max);
float foc_pi_speed(void);
void foc_pi_current(DQ_currents *dq_currents,DQ_Def *dq);
void IPARK(DQ_Def *dq, AlphaBeta_Def *alphaBeta, float sinAngle, float cosAngle); 
//void IPARK(DQ_Def *dq, AlphaBeta_Def *alphaBeta, float angle);
void SVPWM(AlphaBeta_Def *U_alphaBeta, SVPWM_Def *svpwm);
#endif  // IPARK_H 结束头文件保护