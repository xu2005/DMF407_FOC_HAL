cmake_minimum_required(VERSION 3.22)
project(dmf407_host_regression C)
enable_testing()

# Compile the real BSP translation units; only tim.h / adc.h are replaced.
add_executable(bsp_host_tests
    test_bsp.c
    ../../BSP/bsp_encoder.c
    ../../BSP/bsp_adc.c
    ../../BSP/bsp_pwm.c
)
target_include_directories(bsp_host_tests PRIVATE mock ../../BSP)
target_compile_features(bsp_host_tests PRIVATE c_std_11)
if(MSVC)
    target_compile_options(bsp_host_tests PRIVATE /W4)
else()
    target_compile_options(bsp_host_tests PRIVATE -Wall -Wextra -Werror)
endif()
foreach(group encoder encoder_r01 adc pwm)
    add_test(NAME ${group} COMMAND bsp_host_tests ${group})
endforeach()
set_tests_properties(encoder_r01 PROPERTIES LABELS "KNOWN_RISK" TIMEOUT 5)
set_tests_properties(encoder adc pwm PROPERTIES TIMEOUT 5)
