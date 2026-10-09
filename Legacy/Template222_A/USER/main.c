
//#include "usart.h"
#include "stm32f4xx.h"
#include "time1.h"
#include "ipark.h"
#include "svpwm.h"
#include <math.h> 
#include "test.h"
#include <stdio.h>
#include "time2.h"
#include "encoderini.h"
#include "adc.h"
#include "current.h"
#include "globals.h"

#define motorX_period     (TIM4->ARR)//tim4 ��ʱ��
#define motorY_period     (TIM4->ARR)//ARR����
#define motorX_CNT        (TIM4->CNT)//CNT��ǰ����
#define motorY_CNT        (TIM4->CNT)
#define Array_LENGTH       3000
u8 motorX_pluse=0;//�岹
u8 motorY_pluse=0;// X/Y���������
int write_ptr=0,read_ptr=0,dec_ptr=0;// ���λ���ָ�� ��������ɶ����
u8 interpola_start=0;// �岹��ʼ��־
u16 DEC_ACC_PERiod[Array_LENGTH+1]={0};
long speed_period_min=10000,speed_period_max=10000; 
void cul_Saccdec(void);
void data_save(int ptr);
int line_interpola(void);         //�ص��ע
void speed_plane(void);        //�ص��ע
long acc_dec_cnt=0;
double Xscale=2.0,Yscale=2.0;
long Xcoor=0,Ycoor=0;
long pre1_Xcoor=0,pre1_Ycoor=0;
long dec_len=0;
int adjustA_Xlen=1,adjustA_Ylen=2;
const long adjustA[1][2] = {4096,0};


typedef struct _delt_buf
{
  long deltX;
  long deltY;
  u8  accdec;
  long acclen;
}DELT_BUF;
#define DEC 'D'
#define ACC 'A'
#define DataLen 100
DELT_BUF  delt_buf[DataLen]; //

int main (void)
{
      static u8 startStop = 1;
      ADC_Int_Init( ); 
	    Time2_Init(999, 83);//speed huan
	    Time4_Init(speed_period_max, 10);
	    Encoder_Int_Init(0xFFFF,0);  
        TIM1_Int_Init(100-1,84-1);  //10kƵ��  84M/100/84=10k��100us
	   	cul_Saccdec();
	 //F407��ʱ��ʱ��Ϊ84M����Ƶϵ��Ϊ 84�����Լ���Ƶ��
   //Ϊ 84M/84=1Mhz,��װ��ֵ 500������ PWM Ƶ��Ϊ 1M/100=10Khz.

 while(1)
 {
	 int i;
   ADC_ProcessData();
	 
	 if (enable_flag==1) //�岹��ʼ  �����������ٶȻ���ʱ���������κ���
	 {
		 if (read_ptr==write_ptr)
		 {
		   for(i=0;i<adjustA_Xlen;i++)
			 {
			  data_save(i);
			 }
		  speed_plane();
		 }
		 else {
		   line_interpola();
			 if (read_ptr==write_ptr)
				 enable_flag=0;
		 }
	 }
	 
	 
 }
 }

 
void data_save(int ptr)
{
    Xcoor=adjustA[ptr][0];
	  Ycoor=adjustA[ptr][1];
	
     Xcoor=(long)( ((double)Xcoor) * Xscale);
     Ycoor=(long)( ((double)Ycoor) * Yscale);
  
     delt_buf[write_ptr].deltX= Xcoor-pre1_Xcoor; 
     delt_buf[write_ptr].deltY= Ycoor-pre1_Ycoor;
     delt_buf[write_ptr].accdec=ACC;
     pre1_Xcoor=Xcoor;
     pre1_Ycoor=Ycoor;
     
     write_ptr++;

}


void speed_plane(void)
{
  int i;
  long X_delt=0,Y_delt=0,temp_delt=0;
  long declen_sum=0;
  long declen_part=0;
  
  for(i=read_ptr; ;i++)
   {
     if(i==DataLen)  i=0;
     X_delt=(delt_buf[i].deltX>=0)?(delt_buf[i].deltX):(0-delt_buf[i].deltX);
     Y_delt=(delt_buf[i].deltY>=0)?(delt_buf[i].deltY):(0-delt_buf[i].deltY); 
     temp_delt=(X_delt>=Y_delt)?X_delt:Y_delt;
     declen_sum+=temp_delt;

     if(delt_buf[i].accdec==DEC||i==write_ptr)
       {dec_ptr=i;break;}
   }  //==============?????
 
  if(declen_sum>= 2*dec_len)
  {
    for(i=dec_ptr; ;i--)
           {
             X_delt=(delt_buf[i].deltX>=0)?(delt_buf[i].deltX):(0-delt_buf[i].deltX);
             Y_delt=(delt_buf[i].deltY>=0)?(delt_buf[i].deltY):(0-delt_buf[i].deltY); 
             temp_delt=(X_delt>=Y_delt)?X_delt:Y_delt;
             declen_part+=temp_delt;
             delt_buf[i].acclen=0;
             delt_buf[i].accdec=DEC;
             if(declen_part>=dec_len)
             {  delt_buf[i].acclen=declen_part-dec_len;  break;}
             if(i==read_ptr) break;
             if(i==0) i=DataLen;
           }
  }
  else if(declen_sum < 2*dec_len)
  {
    for(i=dec_ptr; ;i--)
           {
             X_delt=(delt_buf[i].deltX>=0)?(delt_buf[i].deltX):(0-delt_buf[i].deltX);
             Y_delt=(delt_buf[i].deltY>=0)?(delt_buf[i].deltY):(0-delt_buf[i].deltY); 
             temp_delt=(X_delt>=Y_delt)?X_delt:Y_delt;
             declen_part+=temp_delt;
             
             delt_buf[i].accdec=DEC;
             delt_buf[i].acclen=0;
            
             if(declen_part>=(declen_sum/2+1)) 
             {  delt_buf[i].acclen=declen_part-(declen_sum/2+1);  break;}
             if(i==read_ptr) break;
             if(i==0) i=DataLen;
           }
  }
  
  dec_ptr++; 
  if(dec_ptr==DataLen) dec_ptr=0;
}
 
  
void motorX_pluse_send(void)
{
   motorX_CNT=0;
   motorX_pluse=1;
   while(motorX_pluse==1){}
}

void motorY_pluse_send(void)
{
   motorY_CNT=0;
   motorY_pluse=1;
   while(motorY_pluse==1){}
}

u16 motor_dec_process(void)
{
  if(acc_dec_cnt>1) acc_dec_cnt--;
  return DEC_ACC_PERiod[acc_dec_cnt];
}

u16 motor_acc_process(int current_tangent)
{
  if(DEC_ACC_PERiod[acc_dec_cnt]>current_tangent) acc_dec_cnt++;
  else if(DEC_ACC_PERiod[acc_dec_cnt]<current_tangent) acc_dec_cnt--;
  return DEC_ACC_PERiod[acc_dec_cnt];
}

int line_interpola(void)
{
  u8 xdir='-',ydir='-';
  long deltaX;
  long deltaY;
  long Xm=0,Ym=0, Fm=0;
  
  deltaX=delt_buf[read_ptr].deltX;
  deltaY=delt_buf[read_ptr].deltY;

 // if(deltaX<0)deltaX=0-deltaX;  //X???
 //   else xdir='+';
 // motorX_dir(xdir);
 // if(deltaY<0)deltaY=0-deltaY;  //Y???
//    else ydir='+';
//  motorY_dir(ydir);
//
  //======================================== ????
  Fm=Ym*deltaX-Xm*deltaY;
	while ((Xm!=deltaX)||(Ym!=deltaY))
	{
		if (acc_dec_cnt==782)
			acc_dec_cnt=acc_dec_cnt;
            if(Fm>=0 && deltaX!=0)
             { 
               if(deltaX>=deltaY)
                   {
                    if(delt_buf[read_ptr].accdec==ACC){
												motorX_period= motor_acc_process(speed_period_min);
                    }
                    else if(delt_buf[read_ptr].accdec==DEC )
                      {
                        if(Xm<delt_buf[read_ptr].acclen){
                          motorX_period= motor_acc_process(speed_period_min);
                        }
                        else{
                          motorX_period= motor_dec_process();
                        }
                      }
                    motorY_period=motorX_period;
                   }
              
              motorX_pluse_send();
              Xm++;
              Fm=Fm-deltaY;
             }
            else if(Fm<=0 && deltaY!=0)
             {
               if(deltaY>deltaX)
                  {  
                    if(delt_buf[read_ptr].accdec==ACC){
                      motorY_period= motor_acc_process(speed_period_min);
                    }
                    if(delt_buf[read_ptr].accdec==DEC )
                      {
                        if(Ym<delt_buf[read_ptr].acclen){
                          motorY_period= motor_acc_process(speed_period_min);
                        }
                        else{ 
                          motorY_period= motor_dec_process();
                        }
                      }
                    motorX_period=motorY_period;
                  }
                 
              motorY_pluse_send();
              Ym++;
              Fm=Fm+deltaX;
             }
         
					 }
  read_ptr++;
  return 0;
}
 

#define m_ieditSlineK  4//S????
#define m_streditMotorStep 1.8//???
#define m_ieditMotordiv    16//??
int show_speed=200;
void cul_Saccdec(void){

    float stepan=360.0/m_streditMotorStep;   //m_streditMotorStep-???
	  int PPR=(int)(60*1000*1000.0/(stepan*m_ieditMotordiv));  //m_ieditMotordiv -???
	  double cpuhz=1.0/0.13925;
	  //float mstomin=1.0/60/1000;
	  double Vmin=5;//rpm
	  double Vmax=730;//rpm
	  double Tmax=300;//75; //Tmax,ms
	   //m_TValue.clear();
	  float flexible=m_ieditSlineK;//????k?,???4
	  int num=0;
	  float fre;
	  float v;
	  float T=0;
	  float melo;
	  float deno;
	  int period=0;
		u8 m_iTimestyle=0;//0:???==???,1:50%?????
		int accCout=0;
    
   Vmin=1;
   if(Vmin<1.0)Vmin=1.0;
   if(Vmin>10.0)Vmin=10.0;
   Tmax=300;
   if(Tmax<30.0)Tmax=30.0;
   if(Tmax>300.0)Tmax=300.0;

  Vmax=(double)show_speed*230.0/Position_Limit;//1200:730
  if(Vmax<=(Vmin+1.0)) Vmax=Vmin+1.0;

     double Ehigh=2.45;  //m_strHigh-??�-2.45us
     double Elow=14;   //m_strLow-???-14
     num=(int)(Tmax*0.5);
     while(T<Tmax)
     {
	melo = flexible * (T-num) / (num);
	deno = 1.0 / (1 + expf(-melo));   
	v= (Vmax-Vmin) * deno + Vmin;   //????
	period=(int)((PPR/v-Ehigh-Elow)*cpuhz);
              if(period>26715) period=26715;
              else if(period<31) period=31;//??3000rpm
	DEC_ACC_PERiod[accCout]=period;  //?????
              accCout++;
	fre=(float)PPR/v;
	T=T+fre/1000.0;
     }
		 
   DEC_ACC_PERiod[accCout]=period-1;//????3000RPM???,68:2000rpm
   //max_DEC_LENGTH=accCout;
		 
		 speed_period_min=DEC_ACC_PERiod[accCout];
		 speed_period_max=DEC_ACC_PERiod[0];
		 dec_len=accCout;
}








 
 
