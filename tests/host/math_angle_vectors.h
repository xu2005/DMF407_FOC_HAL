/* Frozen from unchanged Legacy angle.c by freeze_angle_vectors.py.
 * MinGW GCC, 32-bit long; DUT is not consulted. */
static const struct angle_vector {
    const char *name;
    int32_t previous; uint32_t raw; int32_t total;
    float mechanical, electrical, rpm;
} angle_vectors[] = {
    {"zero", 0, 0U, 0, 5.73654795f, 4.09663677f, 0.0f},
    {"positive", 0, 400U, 400, 0.0816813707f, 0.326725483f, 5999.99951f},
    {"reverse", 400, 399U, 399, 0.0801105499f, 0.3204422f, -15.0f},
    {"sample_1", 0, 100U, 100, 5.89362764f, 4.72495556f, 1499.99988f},
    {"sample_2", 100, 101U, 101, 5.89519835f, 4.73123837f, 15.0f},
    {"upper_wrap", 65535, 0U, 65536, 1.86611319f, 1.18126774f, 15.0f},
    {"lower_wrap", 0, 65535U, -1, 5.73497725f, 4.09035397f, -15.0f},
    {"half_positive_limit", 0, 32767U, 32767, 0.658162534f, 2.63265014f, 491504.938f},
    {"half_positive_wrap", 0, 32768U, 32768, 0.659738004f, 2.63895202f, -491520.0f},
    {"half_negative_limit", 32768, 0U, 0, 5.73654795f, 4.09663677f, -491520.0f},
    {"r01_upper_bad_total", 65535, 0U, 0, 5.73654795f, 4.09663677f, 15.0f},
    {"r01_lower_bad_total", 0, 65535U, 65535, 1.86454153f, 1.17498112f, -15.0f},
};
