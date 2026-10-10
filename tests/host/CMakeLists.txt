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

# Independent pure math target: no HAL mocks and no firmware integration.
add_executable(math_host_tests
    test_math.c
    ../../Motor/motor_angle_speed.c
    ../../Control/foc_transforms.c
)
target_include_directories(math_host_tests PRIVATE ../../Motor ../../Control)
target_compile_features(math_host_tests PRIVATE c_std_11)
if(MSVC)
    target_compile_options(math_host_tests PRIVATE /W4)
else()
    target_compile_options(math_host_tests PRIVATE -Wall -Wextra -Werror)
    target_link_libraries(math_host_tests PRIVATE m)
endif()
foreach(group angle angle_r01 transforms)
    add_test(NAME math_${group} COMMAND math_host_tests ${group})
endforeach()
set_tests_properties(math_angle PROPERTIES FIXTURES_SETUP angle_validated TIMEOUT 5)
set_tests_properties(math_angle_r01 PROPERTIES FIXTURES_REQUIRED angle_validated LABELS "KNOWN_RISK" TIMEOUT 5)
set_tests_properties(math_transforms PROPERTIES FIXTURES_REQUIRED angle_validated TIMEOUT 5)

# Compile actual PI/SVPWM modules; expectations are frozen from Legacy.
add_executable(pi_svpwm_tests
    test_pi_svpwm.c
    ../../Control/foc_pi.c
    ../../Control/foc_svpwm.c
)
target_include_directories(pi_svpwm_tests PRIVATE ../../Control)
target_compile_features(pi_svpwm_tests PRIVATE c_std_11)
if(MSVC)
    target_compile_options(pi_svpwm_tests PRIVATE /W4)
else()
    target_compile_options(pi_svpwm_tests PRIVATE -Wall -Wextra -Werror)
    target_link_libraries(pi_svpwm_tests PRIVATE m)
endif()
foreach(group pi_speed pi_current pi_schedule pi_position svpwm_normal svpwm_zero)
    add_test(NAME ${group} COMMAND pi_svpwm_tests ${group})
    set_tests_properties(${group} PROPERTIES TIMEOUT 5)
endforeach()
set_tests_properties(svpwm_zero PROPERTIES LABELS "KNOWN_RISK")
