#ifndef ENCODER_TRACKER_H
#define ENCODER_TRACKER_H
#include <stdint.h>
typedef struct { int64_t total; uint16_t previous; } Encoder_Tracker;
void Encoder_TrackerReset(Encoder_Tracker *s, uint16_t raw);
/* V2 sampled unwrapping: independent of pending UIF, ISR order and DIR.
 * REQUIRES physical movement between consecutive samples <32768 counts.
 * Exactly half a turn of the counter is ambiguous and rejected. Missed full
 * counter revolutions cannot be detected from CNT alone: L4 bound required.
 * On failure state/output stay unchanged. One owner serializes updates. */
int Encoder_TrackerUpdate(Encoder_Tracker *s, uint16_t raw, int64_t *total);
#endif
