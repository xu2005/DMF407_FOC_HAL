#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "foc_pipeline.h"
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x); exit(1); } } while(0)
#define NEAR(a,b) CHECK(fabsf((a)-(b)) < 0.00002f)
int main(void)
{
    FOC_Pipeline s;
    FOC_PipelineInit(&s);
    CHECK(s.speed_ref == 100 && s.iq_ref == 0 && s.id_ref == 0);
    FOC_PipelineAngleTick(&s, 400, 400);
    NEAR(s.angle.mechanical_angle, .0816813707f);
    NEAR(s.angle.electrical_angle, .326725483f);
    NEAR(s.angle.speed_rpm, 5999.99951f);
    /* Known analytic frame: angle 0, ADC V=0 and W=4095.
     * I_v=-10.4166667, I_w=17.0833333, I_u=-6.6666667;
     * Clarke beta=(I_u+2*I_v)/sqrt(3)=-15.87713.
     * First current PI: d .55*(-I_d), q 1.4*(-I_q) clamped 18.
     * Expectations are constants, not calls to the production stages. */
    FOC_PipelineInit(&s);
    const FOC_RawSample raw = { 4095, 0, 4095 };
    CHECK(FOC_PipelineControlTick(&s, &raw, 1) == FOC_TICK_VALID);
    NEAR(s.v_current, -10.4166667f);
    NEAR(s.w_current, 17.0833321f);
    NEAR(s.u_current, -6.66666555f);
    NEAR(s.bus_voltage, 82.5f);
    NEAR(s.ab_current.I_alpha, -6.66666555f);
    NEAR(s.ab_current.I_beta, -15.8771315f);
    NEAR(s.dq_current.I_d, -6.66666555f);
    NEAR(s.dq_current.I_q, -15.8771315f);
    CHECK(s.id_show==-6 && s.iq_show==-15);
    NEAR(s.current_pi.sum_d, 6.66666555f);
    NEAR(s.current_pi.sum_q, 15.8771315f);
    NEAR(s.dq_voltage.d, 3.66666603f);
    NEAR(s.dq_voltage.q, 18.0f);
    NEAR(s.ab_voltage.alpha, 3.66666603f);
    NEAR(s.ab_voltage.beta, 18.0f);
    CHECK(s.pwm.sector == 1 && s.current_cycle_counter == 1 && s.speed_updates == 0);
    NEAR(s.pwm.ta, 19.27083397f);
    NEAR(s.pwm.tb, 8.76202011f);
    NEAR(s.pwm.tc, 41.2379799f);
    for (unsigned i=2; i<=20; ++i) {
        s.motor_x_position=7; s.motor_y_position=9;
        CHECK(FOC_PipelineControlTick(&s, &raw, 1) == FOC_TICK_VALID);
        CHECK(s.speed_updates == i/10);
        CHECK(s.current_cycle_counter == i%10);
        if (i==10 || i==20) {
            CHECK(s.motor_x_position==0 && s.motor_y_position==0);
            NEAR(s.iq_ref, i==10 ? 8.15f : 8.3f);
            NEAR(s.speed_pi.sum_speed, i==10 ? .15f : .30f);
        }
    }
    float integral=s.current_pi.sum_q;
    CHECK(FOC_PipelineControlTick(&s, &raw, 0)==FOC_TICK_DISABLED);
    CHECK(s.current_cycle_counter==1 && s.current_pi.sum_q==integral);
    FOC_PipelineInit(&s);
    FOC_PipelineAngleTick(&s, 65535, -1);
    NEAR(s.angle.speed_rpm,-15);
    NEAR(s.angle.electrical_angle,4.09035397f);
    /* Exercise the actual SVPWM return-0 path without reading its duties. */
    FOC_PipelineInit(&s);
    CHECK(FOC_PipelineControlTick(&s,&raw,0)==FOC_TICK_DISABLED);
    s.id_ref=s.dq_current.I_d;
    s.iq_ref=s.dq_current.I_q;
    s.pwm.ta=71; s.pwm.tb=73; s.pwm.tc=79;
    CHECK(FOC_PipelineControlTick(&s,&raw,1)==FOC_TICK_INVALID);
    CHECK(s.pwm.sector==0);
    CHECK(s.pwm.ta==71 && s.pwm.tb==73 && s.pwm.tc==79);
    puts("PASS V1 replay: ADC/conversion/angle/transforms/PI/SVPWM, order and disabled state; no realtime claim");
    return 0;
}
