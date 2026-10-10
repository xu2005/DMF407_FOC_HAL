/* Frozen from active Legacy SYSTEM/ipark.c; never from new Control/. */
#ifndef PI_SVPWM_VECTORS_H
#define PI_SVPWM_VECTORS_H
typedef struct {
    float ref;
    float actual;
    float before;
    float after;
    float output;
} SpeedVector;
static const SpeedVector speed_vectors[] = {
    {100.0f, 0.0f, 0.0f, 0.150000006f, 8.14999962f},
    {100.0f, 0.0f, 0.150000006f, 0.300000012f, 8.30000019f},
    {20000.0f, 0.0f, 0.300000012f, 18.0f, 18.0f},
    {0.0f, 0.0f, 18.0f, 18.0f, 18.0f},
    {-20000.0f, 0.0f, 18.0f, -12.0f, -18.0f},
    {-20000.0f, 0.0f, -12.0f, -18.0f, -18.0f},
    {0.0f, 0.0f, -18.0f, -18.0f, -18.0f},
    {50.0f, 25.0f, -18.0f, -17.9624996f, -15.9624996f},
};
typedef struct {
    float id_ref;
    float iq_ref;
    float id_actual;
    float iq_actual;
    float before_d;
    float before_q;
    float after_d;
    float after_q;
    float d;
    float q;
} CurrentVector;
static const CurrentVector current_vectors[] = {
    {0.0f, 0.0f, -2.0f, -3.0f, 0.0f, 0.0f, 2.0f, 3.0f, 1.10000002f, 4.19999981f},
    {0.0f, 0.0f, -2.0f, -3.0f, 2.0f, 3.0f, 4.0f, 6.0f, 1.20000005f, 5.4000001f},
    {0.0f, 0.0f, -100.0f, -100.0f, 4.0f, 6.0f, 18.0f, 18.0f, 18.0f, 18.0f},
    {0.0f, 0.0f, 100.0f, 100.0f, 18.0f, 18.0f, -18.0f, -18.0f, -18.0f, -18.0f},
    {0.0f, 0.0f, 0.0f, 0.0f, -18.0f, -18.0f, -18.0f, -18.0f, -0.900000036f, -7.20000029f},
    {2.0f, 4.0f, 1.0f, 2.0f, -18.0f, -18.0f, -17.0f, -16.0f, -0.350000024f, -4.4000001f},
};
typedef struct {
    float before_speed;
    float before_d;
    float before_q;
    float after_speed;
    float after_d;
    float after_q;
    float iq_ref;
    float d;
    float q;
} ScheduleVector;
static const ScheduleVector schedule_vectors[] = {
    {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f},
    {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f},
    {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f},
    {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f},
    {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f},
    {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f},
    {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f},
    {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f},
    {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f},
    {0.0f, 0.0f, 0.0f, 0.150000006f, 0.0f, 8.14999962f, 8.14999962f, 0.0f, 11.4099998f},
    {0.150000006f, 0.0f, 8.14999962f, 0.150000006f, 0.0f, 16.2999992f, 8.14999962f, 0.0f, 14.6700001f},
    {0.150000006f, 0.0f, 16.2999992f, 0.150000006f, 0.0f, 18.0f, 8.14999962f, 0.0f, 15.3500004f},
    {0.150000006f, 0.0f, 18.0f, 0.150000006f, 0.0f, 18.0f, 8.14999962f, 0.0f, 15.3500004f},
    {0.150000006f, 0.0f, 18.0f, 0.150000006f, 0.0f, 18.0f, 8.14999962f, 0.0f, 15.3500004f},
    {0.150000006f, 0.0f, 18.0f, 0.150000006f, 0.0f, 18.0f, 8.14999962f, 0.0f, 15.3500004f},
    {0.150000006f, 0.0f, 18.0f, 0.150000006f, 0.0f, 18.0f, 8.14999962f, 0.0f, 15.3500004f},
    {0.150000006f, 0.0f, 18.0f, 0.150000006f, 0.0f, 18.0f, 8.14999962f, 0.0f, 15.3500004f},
    {0.150000006f, 0.0f, 18.0f, 0.150000006f, 0.0f, 18.0f, 8.14999962f, 0.0f, 15.3500004f},
    {0.150000006f, 0.0f, 18.0f, 0.150000006f, 0.0f, 18.0f, 8.14999962f, 0.0f, 15.3500004f},
    {0.150000006f, 0.0f, 18.0f, 0.300000012f, 0.0f, 18.0f, 8.30000019f, 0.0f, 15.5f},
};
typedef struct {
    int ref;
    int actual;
    int feed;
    float output;
} PositionVector;
static const PositionVector position_vectors[] = {
    {100, 80, 3, 215.0f},
    {200, 0, 0, 1000.0f},
    {-200, 0, 0, -1000.0f},
    {0, 0, 0, 0.0f},
};
typedef struct {
    float alpha;
    float beta;
    int sector;
    float ta;
    float tb;
    float tc;
} SVPWMVector;
static const SVPWMVector svpwm_vectors[] = {
    {0.0f, 2.0f, 1, 25.0f, 23.1957817f, 26.8042183f},
    {2.0f, -1.0f, 2, 22.9864464f, 27.0135536f, 25.2093353f},
    {2.0f, 1.0f, 3, 22.9864464f, 25.2093353f, 27.0135536f},
    {-2.0f, -1.0f, 4, 27.0135536f, 24.7906647f, 22.9864464f},
    {-2.0f, 1.0f, 5, 27.0135536f, 22.9864464f, 24.7906647f},
    {0.0f, -2.0f, 6, 25.0f, 26.8042183f, 23.1957817f},
    {0.0f, 48.0f, 1, 25.0f, 0.0f, 50.0f},
    {48.0f, -24.0f, 2, 0.0f, 50.0f, 27.5990753f},
    {48.0f, 24.0f, 3, 0.0f, 27.5990753f, 50.0f},
    {-48.0f, -24.0f, 4, 50.0f, 22.4009247f, 0.0f},
    {-48.0f, 24.0f, 5, 50.0f, 0.0f, 22.4009247f},
    {0.0f, -48.0f, 6, 25.0f, 50.0f, 0.0f},
    {2.0f, 0.0f, 2, 23.4375f, 26.5625f, 26.5625f},
    {-2.0f, 0.0f, 4, 26.5625f, 23.4375f, 23.4375f},
};
#endif
