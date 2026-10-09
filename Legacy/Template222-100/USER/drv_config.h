
#ifndef __drv_config_h
#define __drv_config_h

#define ADC_REF_VOLTAGE 3.3
#define ADC_RESOLUTION 4095
#define ENCODER_RESOLUTION  4000  // 
#define PI 3.1415926f  // 定义PI
#define POLE_PAIRS       (4)   // 				// 电机极对数
#define BASE_CURRENT		(16) // 标幺值电流，也是电路板相线电流采集电路能采集的最大电
#define BASE_VOLTAGE    (80) // 82.5，考虑冗余 标幺值电压，也是电路板母线电压采集电路能采集的最大电压
#define MOTO_RS				  (1.02)		// 电机相电阻，单位，欧姆Ohm
#define MOTO_LS					(0.00059)	// 电机相电感，单位，亨H
#define MOTO_KE					(4.3)			// 电机反电动势有效值，单位，Vrms/krpm
#define  MOTO_Speed     (3000)       //电机的最大转速


//电机额定转速3000，机械角度的最大值为100*pi,

//角频率基准值点准的额定频率，
//此外，电阻的标么化 Rbase=Ubase/Ibase;//Lbase=Ubase/（Wbase*Ibase)//
//Tbase=2*PI*Lbase/Rbase;标么化之后做定点化//


//其中，电机的额定电流为4A,标么后的q轴电流可调范围为[0 0.25]
//电机的母线电压为24V,则线电压的最大值为24/sqrt(3)



#endif
