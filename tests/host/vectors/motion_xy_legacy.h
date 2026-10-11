/* Frozen from Legacy data_save/speed_plane/line_interpola and profile.
 * Pulse stubs record axis, ARR and logical XY; no hardware execution.
 * FNV-1a hashes four little-endian uint32 fields per event. */
#include <stdint.h>
static const struct { int32_t x,y; uint32_t count; uint64_t digest; uint32_t final_index; } legacy_xy[] = {
    {0, 0, 0, UINT64_C(14695981039346656037), 0},
    {8, 0, 16, UINT64_C(13993762170848767797), 1},
    {0, 8, 16, UINT64_C(3176340370720434261), 1},
    {3, 2, 10, UINT64_C(8062187638297206722), 1},
    {2, 3, 10, UINT64_C(5546460994149430058), 1},
    {6, 6, 24, UINT64_C(17978481229287296889), 1},
    {4096, 0, 8192, UINT64_C(12778789633138185923), 1},
    {400, 300, 1400, UINT64_C(4974168206955092379), 1},
    {300, 400, 1400, UINT64_C(7781315953657256316), 1},
};
static const struct { uint32_t axis,period; int32_t x,y; } legacy_xy_3_2[] = {
    {0, 26715, 1, 0},
    {1, 26715, 1, 1},
    {0, 26715, 2, 1},
    {1, 26715, 2, 2},
    {0, 26715, 3, 2},
    {0, 26715, 4, 2},
    {1, 26715, 4, 3},
    {0, 26715, 5, 3},
    {1, 26715, 5, 4},
    {0, 26715, 6, 4},
};
