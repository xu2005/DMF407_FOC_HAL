#include "encoder_tracker.h"
#include <limits.h>
void Encoder_TrackerReset(Encoder_Tracker *s, uint16_t raw)
{
    s->total = 0;
    s->previous = raw;
}
int Encoder_TrackerUpdate(Encoder_Tracker *s, uint16_t raw, int64_t *total)
{
    uint16_t modular = (uint16_t)((uint32_t)raw - s->previous);
    if (modular == 32768U) { return 0; }
    int32_t delta = modular <= 32767U ? (int32_t)modular : (int32_t)modular - 65536;
    if ((delta > 0 && s->total > INT64_MAX - delta) ||
        (delta < 0 && s->total < INT64_MIN - delta)) { return 0; }
    s->total += delta;
    s->previous = raw;
    *total = s->total;
    return 1;
}
