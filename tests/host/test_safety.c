#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tim.h"
#include "adc.h"
#include "main.h"
#include "bsp_adc_safe.h"
#include "bsp_encoder_safe.h"
#include "bsp_safety.h"
#include "bsp_timer.h"
#include "app_no_power.h"
#include "tim1_model.h"
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x); exit(1); } } while(0)
static TIM_TypeDef regs[4];
static Tim1_Model tim1_model;
TIM_HandleTypeDef htim1={&regs[0]},htim2={&regs[1]},htim3={&regs[2]},htim4={&regs[3]};
ADC_HandleTypeDef hadc1={1},hadc2={2},hadc3={3};
uint32_t mock_primask;
GPIO_TypeDef mock_gpiof;
static int start_fail=-1,stop_fail=-1,adc_fail=-1,poll_fail=-1,timer_fail=-1,encoder_fail;
static unsigned starts,stops,adc_starts,adc_stops,polls,reads,base_starts,gate_writes;
static uint32_t time_ms;
static HAL_StatusTypeDef injected=HAL_ERROR;
static unsigned read_ids[6];
void mock_clear_bits(uint32_t *reg, uint32_t bits)
{
    if (reg == &regs[0].BDTR) { Tim1_ModelWriteBDTR(&tim1_model, *reg & ~bits); }
    else { *reg &= ~bits; }
}
static int discard_bad;
static const uint16_t values[]={17,19,23,4095,0,4095};
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *h);
uint32_t HAL_GetTick(void) { return time_ms; }
void mock_record(enum mock_event_kind k,const void *h,uint32_t v) { (void)k;(void)h;(void)v; }
void HAL_GPIO_WritePin(GPIO_TypeDef *port,uint16_t pin,GPIO_PinState state)
{
    CHECK(port==&mock_gpiof && pin==0x400 && state==GPIO_PIN_RESET);
    mock_gpiof.ODR &= ~pin;
    ++gate_writes;
}
void BSP_Encoder_OverflowTick(void) { CHECK(0); } /* App must choose V2 dispatch. */
uint32_t *mock_compare_register(TIM_HandleTypeDef *h,uint32_t c)
{
    CHECK(h==&htim1);
    if(c==TIM_CHANNEL_1) return &h->Instance->CCR1;
    if(c==TIM_CHANNEL_2) return &h->Instance->CCR2;
    CHECK(c==TIM_CHANNEL_3);return &h->Instance->CCR3;
}
static HAL_StatusTypeDef start(TIM_HandleTypeDef *h,uint32_t c,unsigned complementary)
{
    CHECK(h==&htim1 && gate_writes>0);
    CHECK(c==4U*(starts%6/2) && complementary==starts%2);
    Tim1_ModelWriteBDTR(&tim1_model, regs[0].BDTR | TIM_BDTR_MOE);
    regs[0].CCER|=(complementary?4U:1U) << c;regs[0].CR1|=TIM_CR1_CEN;
    unsigned index=starts++;
    return (int)index==start_fail?injected:HAL_OK;
}
HAL_StatusTypeDef HAL_TIM_PWM_Start(TIM_HandleTypeDef *h,uint32_t c) { return start(h,c,0); }
HAL_StatusTypeDef HAL_TIMEx_PWMN_Start(TIM_HandleTypeDef *h,uint32_t c) { return start(h,c,1); }
static HAL_StatusTypeDef stop(TIM_HandleTypeDef *h,uint32_t c)
{
    (void)c;CHECK(h==&htim1 && gate_writes>0);
    /* Gate checks happen at the HAL boundary, before a stop can succeed. */
    CHECK(!(regs[0].BDTR&TIM_BDTR_MOE)); /* AOE may be locked at 1 in rejected configuration. */
    CHECK((regs[0].CCER&0x555)==0 && !(regs[0].CR1&TIM_CR1_CEN));
    CHECK(!(regs[0].DIER&TIM_IT_UPDATE));
    unsigned index=stops++;
    return (int)index==stop_fail?injected:HAL_OK;
}
HAL_StatusTypeDef HAL_TIM_PWM_Stop(TIM_HandleTypeDef *h,uint32_t c) { return stop(h,c); }
HAL_StatusTypeDef HAL_TIMEx_PWMN_Stop(TIM_HandleTypeDef *h,uint32_t c) { return stop(h,c); }
HAL_StatusTypeDef HAL_ADC_Start(ADC_HandleTypeDef *h)
{
    CHECK(h->id==adc_starts+1);
    unsigned index=adc_starts++;
    return (int)index==adc_fail?injected:HAL_OK;
}
HAL_StatusTypeDef HAL_ADC_Stop(ADC_HandleTypeDef *h) { (void)h;++adc_stops;return injected; }
HAL_StatusTypeDef HAL_ADC_PollForConversion(ADC_HandleTypeDef *h,uint32_t timeout)
{
    const unsigned order[]={3,2,1,3,2,1};
    CHECK(h->id==order[polls%6] && timeout!=HAL_MAX_DELAY);
    unsigned index=polls++;
    return (int)index==poll_fail?injected:HAL_OK;
}
uint32_t HAL_ADC_GetValue(ADC_HandleTypeDef *h)
{
    CHECK(reads<polls);
    read_ids[reads%6]=h->id;
    uint16_t v=values[reads++%6];
    return discard_bad?5000U:v;
}
HAL_StatusTypeDef HAL_TIM_Encoder_Stop(TIM_HandleTypeDef *h,uint32_t c)
{
    CHECK(h==&htim3 && c==TIM_CHANNEL_ALL);regs[2].CR1&=~TIM_CR1_CEN;return encoder_fail==2?injected:HAL_OK;
}
HAL_StatusTypeDef HAL_TIM_Encoder_Start(TIM_HandleTypeDef *h,uint32_t c)
{
    CHECK(h==&htim3 && c==TIM_CHANNEL_ALL);
    regs[2].CR1|=TIM_CR1_CEN;return encoder_fail==1?injected:HAL_OK;
}
HAL_StatusTypeDef HAL_TIM_Base_Start_IT(TIM_HandleTypeDef *h)
{
    unsigned index=base_starts++;
    h->Instance->CR1|=TIM_CR1_CEN;h->Instance->DIER|=TIM_IT_UPDATE;
    return (int)index==timer_fail?injected:HAL_OK;
}
HAL_StatusTypeDef HAL_TIM_Base_Stop_IT(TIM_HandleTypeDef *h)
{
    h->Instance->CR1&=~TIM_CR1_CEN;h->Instance->DIER&=~TIM_IT_UPDATE;return injected;
}
static void fault_check(BSP_Fault reason)
{
    CHECK(BSP_Safety_GetFault()==reason);
    CHECK(!(regs[0].BDTR&TIM_BDTR_MOE) && (regs[0].CCER&0x555)==0);
    CHECK(!(regs[0].DIER&TIM_IT_UPDATE) && gate_writes>0);
    CHECK(!(mock_gpiof.ODR&PM1_CTRL_SD_Pin));
    unsigned before=starts;
    CHECK(!BSP_PWM_StartChecked() && starts==before);
    unsigned base_before=base_starts,adc_before=adc_starts;
    CHECK(!BSP_TIM1_StartDiagnosticChecked() && !BSP_Timer_StartTim1Diagnostic());
    CHECK(!BSP_Timer_StartChecked() && base_starts==base_before);
    CHECK(!BSP_ADC_StartChecked() && adc_starts==adc_before);
    CHECK(!BSP_EncoderSafe_Start() && !App_NoPower_Start());
    BSP_Safety_Fault(BSP_FAULT_INIT);
    CHECK(BSP_Safety_GetFault()==reason); /* Preserve original fault. */
    Tim1_ModelUpdate(&tim1_model);
    CHECK((regs[0].CCER&0x555)==0 && !(regs[0].CR1&TIM_CR1_CEN));
    if (!(regs[0].BDTR&TIM_BDTR_AOE)) { CHECK(!(regs[0].BDTR&TIM_BDTR_MOE)); }
}
static void encoder_case(void)
{
    CHECK(BSP_EncoderSafe_Start());
    uint16_t raw;int64_t total;
    const uint16_t path[]={20000,40000,65535,0,65535,0,1,0,65535,65534};
    const int64_t expected[]={20000,40000,65535,65536,65535,65536,65537,65536,65535,65534};
    for(unsigned i=0;i<10;++i) {
        regs[2].CNT=path[i];regs[2].SR=TIM_FLAG_UPDATE;
        regs[2].CR1=(i%2)?TIM_CR1_DIR:0; /* DIR may change before pending ISR. */
        if(i%3==0) BSP_EncoderSafe_OverflowTick();
        CHECK(BSP_EncoderSafe_Sample(&raw,&total) && raw==path[i] && total==expected[i]);
        if(i%3!=0) BSP_EncoderSafe_OverflowTick();
        CHECK(BSP_EncoderSafe_Sample(&raw,&total) && total==expected[i]);
    }
    CHECK(BSP_EncoderSafe_Start());
    CHECK(regs[2].CNT==0 && !(regs[2].SR&TIM_FLAG_UPDATE));
    CHECK(BSP_EncoderSafe_GetOverflowEvents()==0);
    regs[2].CNT=65535;
    CHECK(BSP_EncoderSafe_Sample(&raw,&total) && total==-1);
    regs[2].CNT=32767;
    CHECK(!BSP_EncoderSafe_Sample(&raw,&total) && total==-1);
}
int main(int argc,char **argv)
{
    CHECK(argc>=2);
    int index=argc>2?atoi(argv[2]):-1;
    if(argc>3) injected=(HAL_StatusTypeDef)atoi(argv[3]);
    Tim1_ModelReset(&tim1_model,&regs[0]);
    Tim1_ModelWriteBDTR(&tim1_model,0x3154U); /* DTG84, LOCK1, BKE/BKP, AOE0; matches MX_TIM1_Init. */
    regs[0].ARR=99;regs[0].CCR1=7;regs[0].CCR2=11;regs[0].CCR3=19;
    if(!strcmp(argv[1],"pwm_start")) {
        start_fail=index;
        CHECK(!BSP_PWM_StartChecked() && starts==(unsigned)index+1 && stops==6);
        fault_check(BSP_FAULT_PWM_START);
    } else if(!strcmp(argv[1],"pwm_stop")) {
        CHECK(BSP_PWM_StartChecked());regs[0].DIER=TIM_IT_UPDATE;
        stop_fail=index;
        CHECK(!BSP_PWM_StopChecked() && stops==6);
        fault_check(BSP_FAULT_PWM_STOP);
    } else if(!strcmp(argv[1],"adc_start")) {
        adc_fail=index;
        CHECK(!BSP_ADC_StartChecked() && adc_starts==(unsigned)index+1 && adc_stops==3);
        fault_check(BSP_FAULT_ADC_START);
    } else if(!strcmp(argv[1],"adc_poll")) {
        CHECK(BSP_ADC_StartChecked());poll_fail=index;
        FOC_RawSample out={100,200,300};
        CHECK(!BSP_ADC_ReadAllChecked(&out,2));
        CHECK(out.bus==100 && out.v==200 && out.w==300);
        CHECK(polls==(unsigned)index+1 && reads==(unsigned)index && adc_stops==3);
        fault_check(BSP_FAULT_ADC_READ);
    } else if(!strcmp(argv[1],"adc_bad")) {
        FOC_RawSample out={100,200,300};
        if(index!=0) CHECK(BSP_ADC_StartChecked());
        discard_bad=index==3;
        CHECK(!BSP_ADC_ReadAllChecked(index==2?NULL:&out,index==1?HAL_MAX_DELAY:2));
        CHECK(out.bus==100 && out.v==200 && out.w==300);
        fault_check(BSP_FAULT_ADC_READ);
    } else if(!strcmp(argv[1],"svpwm_bad")) {
        FOC_SVPWMResult p={1,13,47,83};
        if(index==0)p.sector=0;
        if(index==1)p.ta=NAN;
        if(index==2)p.tb=INFINITY;
        if(index==3)p.tc=-1;
        if(index==4)p.ta=99.1f;
        if(index==5)p.sector=7;
        if(index==6)p.tc=-INFINITY;
        CHECK(!BSP_PWM_ApplyChecked(index==7?NULL:&p));
        CHECK(regs[0].CCR1==7 && regs[0].CCR2==11 && regs[0].CCR3==19);
        fault_check(BSP_FAULT_CONTROL);
    } else if(!strcmp(argv[1],"encoder")) {
        encoder_case();
    } else if(!strcmp(argv[1],"early_init")) {
        htim1.Instance=NULL;BSP_Safety_Fault(BSP_FAULT_INIT);
        CHECK(BSP_Safety_GetFault()==BSP_FAULT_INIT && gate_writes>0);
    } else if(!strcmp(argv[1],"app_failure")) {
        if(index==0)timer_fail=0;
        if(index==1)timer_fail=1;
        if(index==2)encoder_fail=1;
        if(index==3)encoder_fail=2;
        CHECK(!App_NoPower_Start());
        fault_check(index>=2?BSP_FAULT_ENCODER:BSP_FAULT_TIMER);
        if(index<2) CHECK(!(regs[1].CR1&TIM_CR1_CEN) && !(regs[3].CR1&TIM_CR1_CEN));
    } else if(!strcmp(argv[1],"app_replay")) {
        CHECK(App_NoPower_Start());
        CHECK(regs[0].DIER&TIM_IT_UPDATE);
#if !DMF407_NO_POWER_CONTROL
        CHECK(starts==0 && (regs[0].CCER&0x555)==0 && !(regs[0].BDTR&TIM_BDTR_MOE));
        CHECK(regs[0].CR1&TIM_CR1_CEN);
#endif
        App_NoPower_Poll();CHECK(reads==6);
        HAL_TIM_PeriodElapsedCallback(&htim2);
        HAL_TIM_PeriodElapsedCallback(&htim3);
        CHECK(BSP_EncoderSafe_GetOverflowEvents()==1);
        for(unsigned i=0;i<10;++i) HAL_TIM_PeriodElapsedCallback(&htim1);
        FOC_Pipeline s;App_NoPower_GetSnapshot(&s);
#if DMF407_NO_POWER_CONTROL
        CHECK(s.speed_updates==1 && s.current_cycle_counter==0);
        CHECK(s.raw.bus==4095 && s.raw.v==0 && s.raw.w==4095 && s.pwm.sector>=1 && s.pwm.sector<=6);
        CHECK(regs[0].CCR1==(uint16_t)s.pwm.ta && regs[0].CCR2==(uint16_t)s.pwm.tb && regs[0].CCR3==(uint16_t)s.pwm.tc);
        CHECK(gate_writes>=12);
        time_ms=21;
        HAL_TIM_PeriodElapsedCallback(&htim1);
        fault_check(BSP_FAULT_STALE);
#else
        CHECK(s.speed_updates==0 && s.current_cycle_counter==0);
        CHECK(regs[0].CCR1==7 && regs[0].CCR2==11 && regs[0].CCR3==19);
        time_ms=3;
        HAL_TIM_PeriodElapsedCallback(&htim2);
        fault_check(BSP_FAULT_ENCODER);
#endif
        CHECK(!App_NoPower_Start());
    } else if(!strcmp(argv[1],"valid")) {
        CHECK(BSP_ADC_StartChecked());FOC_RawSample out;
        CHECK(BSP_ADC_ReadAllChecked(&out,2));
        CHECK(out.bus==4095 && out.v==0 && out.w==4095 && reads==6);
        const unsigned order[]={3,2,1,3,2,1};
        for(unsigned i=0;i<6;++i) CHECK(read_ids[i]==order[i]);
        CHECK(BSP_PWM_StartChecked() && starts==6);
        FOC_SVPWMResult p={1,13.9f,47.2f,99};
        CHECK(BSP_PWM_ApplyChecked(&p));
        CHECK(regs[0].CCR1==13 && regs[0].CCR2==47 && regs[0].CCR3==99);
        CHECK(BSP_PWM_StopChecked());
        CHECK(BSP_Safety_GetFault()==BSP_FAULT_NONE);
    } else if(!strcmp(argv[1],"locked_aoe")) {
        Tim1_ModelReset(&tim1_model,&regs[0]);
        Tim1_ModelWriteBDTR(&tim1_model,0xf154U);
        regs[0].CCER=0x555;regs[0].CR1=TIM_CR1_CEN;
        Tim1_ModelWriteBDTR(&tim1_model,regs[0].BDTR & ~(TIM_BDTR_AOE|TIM_BDTR_MOE));
        CHECK((regs[0].BDTR&TIM_BDTR_AOE) && !(regs[0].BDTR&TIM_BDTR_MOE));
        Tim1_ModelUpdate(&tim1_model);CHECK(regs[0].BDTR&TIM_BDTR_MOE); /* Reproduces old defect. */
        CHECK(!App_NoPower_Start() && starts==0 && adc_starts==0);
        fault_check(BSP_FAULT_TIM1_CONFIG);
        CHECK((regs[0].BDTR&TIM_BDTR_AOE) && (regs[0].BDTR&TIM_BDTR_MOE));
    } else if(!strcmp(argv[1],"lock_write")) {
        uint32_t initial=regs[0].BDTR;
        Tim1_ModelWriteBDTR(&tim1_model,0xffffU);
        CHECK((regs[0].BDTR&0x73ffU)==(initial&0x73ffU));
        CHECK(regs[0].BDTR&TIM_BDTR_MOE);
        Tim1_ModelWriteBDTR(&tim1_model,0);
        CHECK((regs[0].BDTR&0x73ffU)==(initial&0x73ffU));
        CHECK(!(regs[0].BDTR&TIM_BDTR_MOE));
    } else if(!strcmp(argv[1],"break_update")) {
        CHECK(BSP_PWM_StartChecked());Tim1_ModelBreak(&tim1_model,1);
        CHECK(!(regs[0].BDTR&TIM_BDTR_MOE));
        Tim1_ModelBreak(&tim1_model,0);Tim1_ModelUpdate(&tim1_model);
        CHECK(!(regs[0].BDTR&TIM_BDTR_MOE));
        BSP_Safety_Fault(BSP_FAULT_CONTROL);fault_check(BSP_FAULT_CONTROL);
    } else if(!strcmp(argv[1],"initial_moe")) {
        Tim1_ModelWriteBDTR(&tim1_model,regs[0].BDTR|TIM_BDTR_MOE);
        regs[0].CCER=0x555;
        CHECK(BSP_TIM1_StartDiagnosticChecked() && starts==0);
        CHECK(!(regs[0].BDTR&TIM_BDTR_MOE) && !(regs[0].CCER&0x555));
        Tim1_ModelUpdate(&tim1_model);CHECK(!(regs[0].BDTR&TIM_BDTR_MOE));
    } else if(!strcmp(argv[1],"wrong_lock")) {
        Tim1_ModelReset(&tim1_model,&regs[0]);
        Tim1_ModelWriteBDTR(&tim1_model,0x3054U);
        CHECK(!BSP_PWM_StartChecked() && starts==0);fault_check(BSP_FAULT_TIM1_CONFIG);
    } else if(!strcmp(argv[1],"adc_previous")) {
        CHECK(BSP_ADC_StartChecked());FOC_RawSample out;
        CHECK(BSP_ADC_ReadAllChecked(&out,2));poll_fail=11;injected=HAL_TIMEOUT;
        CHECK(!BSP_ADC_ReadAllChecked(&out,2));
        CHECK(out.bus==4095 && out.v==0 && out.w==4095);
        fault_check(BSP_FAULT_ADC_READ);
    } else if(!strcmp(argv[1],"off_timeout")) {
        CHECK(App_NoPower_Start());poll_fail=0;injected=HAL_TIMEOUT;
        App_NoPower_Poll();CHECK(starts==0);fault_check(BSP_FAULT_ADC_READ);
    } else if(!strcmp(argv[1],"encoder_ambiguous")) {
        CHECK(App_NoPower_Start());regs[2].CNT=32768;
        HAL_TIM_PeriodElapsedCallback(&htim2);fault_check(BSP_FAULT_ENCODER);
    } else if(!strcmp(argv[1],"sample_age")) {
        CHECK(App_NoPower_Start());App_NoPower_Poll();
        for(time_ms=0;time_ms<=20;++time_ms) { HAL_TIM_PeriodElapsedCallback(&htim2); }
        time_ms=20;
        HAL_TIM_PeriodElapsedCallback(&htim1);CHECK(BSP_Safety_GetFault()==BSP_FAULT_NONE);
        time_ms=21;HAL_TIM_PeriodElapsedCallback(&htim2); /* Fresh encoder, old ADC sample. */
        HAL_TIM_PeriodElapsedCallback(&htim1);fault_check(BSP_FAULT_STALE);
    } else if(!strcmp(argv[1],"app_adc_transaction")) {
        CHECK(App_NoPower_Start());App_NoPower_Poll();HAL_TIM_PeriodElapsedCallback(&htim2);
        HAL_TIM_PeriodElapsedCallback(&htim1);FOC_Pipeline before,after;
        App_NoPower_GetSnapshot(&before);CHECK(before.raw.bus==4095);
        poll_fail=11;injected=HAL_TIMEOUT;App_NoPower_Poll();
        HAL_TIM_PeriodElapsedCallback(&htim1);App_NoPower_GetSnapshot(&after);
        CHECK(memcmp(&before,&after,sizeof(before))==0);fault_check(BSP_FAULT_ADC_READ);
    } else { CHECK(0); }
    puts("PASS checked path with bounded BDTR LOCK/Break/Update model; NEEDS HARDWARE");
    return 0;
}
