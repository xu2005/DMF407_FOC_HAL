/* Frozen Legacy positive multi-segment speed_plane/interpolation traces. */
#include <stdint.h>
static const int32_t legacy_batch_0[][2] = {{2,1},{2,1},{4,3},{5,5}};
static const int32_t legacy_batch_1[][2] = {{150,50},{160,60},{180,100},{400,300}};
static const int32_t legacy_batch_2[][2] = {{50,150},{60,160},{100,180},{300,400}};
static const struct { const int32_t (*points)[2]; uint32_t segments,count; uint64_t digest; uint32_t final_index; } legacy_batches[] = {
    {legacy_batch_0, 4, 20, UINT64_C(4240599691819600999), 1},
    {legacy_batch_1, 4, 1400, UINT64_C(13198510330083363454), 1},
    {legacy_batch_2, 4, 1400, UINT64_C(2541448530719478695), 1},
};
