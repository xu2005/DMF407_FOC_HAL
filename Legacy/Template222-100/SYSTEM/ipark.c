#include "ipark.h"
#include <math.h> // 包含数学库以使用cos和sin函数
 // 声明外部变量
 #include <stdio.h>
 #include "time1.h"
#include "globals.h"
#include "drv_config.h"
float tcm1 = 0.0f;
float tcm2 = 0.0f;
float tcm3 = 0.0f;
float q_current;

void clarkTransform(AlphaBeta_Currents *ab_currents)
{
    ab_currents->I_alpha = u_current;
    ab_currents->I_beta = (u_current+ 2 * v_current) / 1.73205080757f;
}
void parkTransform(AlphaBeta_Currents *ab_currents, float sinAngle, float cosAngle, DQ_currents *dq_currents) {
    dq_currents->I_q = -ab_currents->I_alpha * sinAngle + ab_currents->I_beta * cosAngle;
	q_current=dq_currents->I_q;
	
    dq_currents->I_d = ab_currents->I_alpha * cosAngle + ab_currents->I_beta * sinAngle;
}

void clamp(float *value, float min, float max)
{
    if (*value > max) {
        *value = max;
    } else if (*value < min) {
        *value = min;
    }
}


void foc_pi_position(long Position_Ref,long Position_feed)
{
	float err_position=Position_Ref_X-Position_actual;
	Speed_Ref=Position_Kp*err_position+Position_Kff*Position_feed;
	clamp(&Speed_Ref,-Position_Limit, Position_Limit);
}


float foc_pi_speed(void)
{
	/*
	if(Speed_Ref==0.0f)
	{
		
		 sum_speed=0;
     sum_d=0;
     sum_q=0;	
		return 0; // 返回0表示没有参考值;
	}
	else
		{
		*/
	float err_speed=Speed_Ref-Speed_actual;
	float sum_speed_local = sum_speed; // 使用局部变量保存累计值
	  sum_speed_local+=Speed_Ki*err_speed;
	 clamp(&sum_speed_local,Speed_Imin, Speed_Imax);
	iq_Ref=Speed_Kp*err_speed+sum_speed_local;
	clamp(&iq_Ref,Speed_Imin, Speed_Imax);
	sum_speed = sum_speed_local;
	
	return iq_Ref;
//}
		}

long id_show=0,iq_show=0;
void foc_pi_current(DQ_currents *dq_currents, DQ_Def *dq)
{
 id_show=dq_currents->I_d;
 iq_show=dq_currents->I_q;
	
    float err_d = (id_Ref - dq_currents->I_d);
	   float sum_d_local = sum_d; // 使用局部变量保存累计值
    float err_q = (iq_Ref - dq_currents->I_q);
	  float sum_q_local = sum_q; // 使用局部变量保存累计值
	
  // d轴电流控制 
   sum_d_local += err_d;
    clamp(&sum_d_local, current_Umin, current_Umax);
    dq->d = (id_Kp * err_d + id_Ki * sum_d_local);//d轴电压值
    clamp(&dq->d, current_Umin, current_Umax);
	  sum_d = sum_d_local; // 保存积分结果到全局变量
 // q轴电流控制
    sum_q_local += err_q;
    clamp(&sum_q_local, current_Umin, current_Umax);
    dq->q =(iq_Kp * err_q + iq_Ki * sum_q_local);   ////q轴电压值
    clamp(&dq->q, current_Umin, current_Umax);
	  sum_q = sum_q_local; // 保存积分结果到全局变量
}
	

void IPARK(DQ_Def *dq, AlphaBeta_Def *alphaBeta, float sinAngle, float cosAngle) 
	{
    alphaBeta->alpha = dq->d * cosAngle - dq->q * sinAngle;
    alphaBeta->beta = dq->d * sinAngle + dq->q * cosAngle;
}
	


//void IPARK(DQ_Def *dq, AlphaBeta_Def *alphaBeta, float angle) 
//{

//    // 计算角度的余弦和正弦值
//    float cosAngle = cos(angle);
//    float sinAngle = sin(angle);

//    // 进行逆Park变换，计算α和β轴上的分量
//    alphaBeta->alpha = dq->d * cosAngle - dq->q * sinAngle;
//    alphaBeta->beta = dq->d * sinAngle + dq->q * cosAngle;
//}

void SVPWM(AlphaBeta_Def *U_alphaBeta, SVPWM_Def *svpwm)
{
    float sum;
    float k_svpwm;
   float Udc=24;
	   svpwm->Ts = 100.0f; // SVPWM的采样周期
	  float K=0.8660254f/Udc*svpwm->Ts;

 
    svpwm->U_alpha = U_alphaBeta->alpha;
    svpwm->U_beta = U_alphaBeta->beta;

    svpwm->u1 =U_alphaBeta->beta;// sqrt(3)/2 ≈ 0.86603
    svpwm->u2 = 0.8660254f * U_alphaBeta->alpha - 0.5f * U_alphaBeta->beta;
    svpwm->u3 = -0.8660254f * U_alphaBeta->alpha - 0.5f * U_alphaBeta->beta;
	
	
	  svpwm->X =  K*svpwm->u1;// sqrt(3)/2 ≈ 0.86603
    svpwm->Y= K*svpwm->u3;
    svpwm->Z= K* svpwm->u2;
	

    svpwm->sector = (svpwm->u1 > 0.0f) + ((svpwm->u2 > 0.0f) << 1) + ((svpwm->u3 > 0.0f) << 2);
  
switch ( svpwm->sector)
{

case 5:

// 扇区5

     svpwm->t4 = svpwm->X;
     svpwm->t6 = svpwm->Y;
      sum = svpwm->X + svpwm->Y;
if (sum > svpwm->Ts)
{
       k_svpwm = svpwm->Ts / sum; 
     svpwm->t4 = k_svpwm * svpwm->t4;
       svpwm->t6 = k_svpwm * svpwm->t6;//过调制

}
//svpwm->t7 = (svpwm->Ts - svpwm->t4 - svpwm->t6)*0.5;

//ta为TCM1 tb为TCM2 tc为TCM3
svpwm->ta=(svpwm->Ts+svpwm->t4 + svpwm->t6)*0.25f;
svpwm->tb =(svpwm->Ts - svpwm->t4 - svpwm->t6)*0.25f;
svpwm->tc =(svpwm->Ts+svpwm->t4 - svpwm->t6)*0.25f;

 //切换时间

break;

case 1:

// 扇区1
svpwm->t4 = - svpwm->Z;

svpwm->t6 = - svpwm->Y;

sum = svpwm->t4 + svpwm->t6;

     if (sum > svpwm->Ts)

{
         k_svpwm = svpwm->Ts / sum; // 计算缩放系数
         svpwm->t4= k_svpwm * svpwm->t4;
          svpwm->t6 = k_svpwm * svpwm->t6;
}

//         svpwm->t7 = (svpwm->Ts - svpwm->t2 - svpwm->t6)*0.5;
         svpwm->ta = (svpwm->Ts+svpwm->t4 - svpwm->t6)*0.25f;
         svpwm->tb = (svpwm->Ts - svpwm->t4 - svpwm->t6)*0.25f;
          svpwm->tc = (svpwm->Ts+svpwm->t4 + svpwm->t6)*0.25f;

break;

case 3:

// 扇区3

         svpwm->t4= svpwm->Z;
         svpwm->t6 = svpwm->X;

         sum = svpwm->t4 + svpwm->t6;
if (sum > svpwm->Ts)

{

    k_svpwm = svpwm->Ts / sum; //

    svpwm->t4 = k_svpwm * svpwm->t4;

    svpwm->t6 = k_svpwm * svpwm->t6;
}

//svpwm->t7 = (svpwm->Ts - svpwm->t2 - svpwm->t3)*0.5;

svpwm->ta = (svpwm->Ts - svpwm->t4 - svpwm->t6)*0.25f;

svpwm->tb = (svpwm->Ts + svpwm->t4 - svpwm->t6)*0.25f;
svpwm->tc = (svpwm->Ts +svpwm->t4 + svpwm->t6)*0.25f; 
break;

case 2:

// 扇区2
svpwm->t4 = -svpwm->Y;

svpwm->t6= -svpwm->X;

sum = svpwm->t4+ svpwm->t6;
if (sum > svpwm->Ts)

{

k_svpwm = svpwm->Ts / sum; //

svpwm->t4 = k_svpwm * svpwm->t4;
svpwm->t6 = k_svpwm * svpwm->t6;
}
//svpwm->t7 = (svpwm->Ts - svpwm->t1 - svpwm->t3)*0.5;
svpwm->ta = (svpwm->Ts - svpwm->t4 - svpwm->t6)*0.25f;

svpwm->tb = (svpwm->Ts + svpwm->t4 +svpwm->t6)*0.25f;
svpwm->tc = (svpwm->Ts + svpwm->t4 - svpwm->t6)*0.25f; 

break;



case 6:// 扇区6

svpwm->t4 = svpwm->Y;

svpwm->t6 = svpwm->Z;

sum = svpwm->t4 + svpwm->t6;

if (sum > svpwm->Ts)

{

k_svpwm = svpwm->Ts / sum; //

svpwm->t4 = k_svpwm * svpwm->t4;

svpwm->t6 = k_svpwm * svpwm->t6;
}

//svpwm->t7 = (svpwm->Ts - svpwm->t1 - svpwm->t5)*0.5;


svpwm->ta = (svpwm->Ts + svpwm->t4 - svpwm->t6)*0.25f;

svpwm->tb = (svpwm->Ts + svpwm->t4 +svpwm->t6)*0.25f;

svpwm->tc = (svpwm->Ts - svpwm->t4 - svpwm->t6)*0.25f; 

break;



case 4:

// 扇区4

svpwm->t4 = -svpwm->X;

svpwm->t6 = -svpwm->Z;

sum = svpwm->t4 + svpwm->t6;

if (sum > svpwm->Ts)

{

    k_svpwm = svpwm->Ts / sum; //

     svpwm->t4 = k_svpwm * svpwm->t4;

        svpwm->t6= k_svpwm * svpwm->t6;

}

//        svpwm->t7 = (svpwm->Ts - svpwm->t4 - svpwm->t5)*0.5;

          svpwm->ta =(svpwm->Ts + svpwm->t4+svpwm->t6)*0.25f;
          svpwm->tb = (svpwm->Ts + svpwm->t4 - svpwm->t6)*0.25f;
          svpwm->tc = (svpwm->Ts - svpwm->t4 - svpwm->t6)*0.25f;

break;

default:

break;
}
       tcm1=svpwm->ta;
       tcm2=svpwm->tb;
       tcm3=svpwm->tc;

  
      
  }   
   
  
    