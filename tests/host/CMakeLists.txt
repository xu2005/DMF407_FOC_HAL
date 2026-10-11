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

add_executable(timer_host_tests test_timer.c ../../BSP/bsp_timer.c)
target_include_directories(timer_host_tests PRIVATE mock ../../BSP ../../Control)
target_compile_features(timer_host_tests PRIVATE c_std_11)
if(MSVC)
    target_compile_options(timer_host_tests PRIVATE /W4)
else()
    target_compile_options(timer_host_tests PRIVATE -Wall -Wextra -Werror)
endif()
add_test(NAME timer_diagnostic COMMAND timer_host_tests)
set_tests_properties(timer_diagnostic PROPERTIES TIMEOUT 5)

add_executable(pipeline_host_tests test_pipeline.c
    ../../Control/foc_pipeline.c ../../Control/foc_transforms.c
    ../../Control/foc_pi.c ../../Control/foc_svpwm.c ../../Motor/motor_angle_speed.c)
target_include_directories(pipeline_host_tests PRIVATE ../../Control ../../Motor)
target_compile_features(pipeline_host_tests PRIVATE c_std_11)
if(MSVC)
    target_compile_options(pipeline_host_tests PRIVATE /W4)
else()
    target_compile_options(pipeline_host_tests PRIVATE -Wall -Wextra -Werror)
    target_link_libraries(pipeline_host_tests PRIVATE m)
endif()
add_test(NAME pipeline_v1_replay COMMAND pipeline_host_tests)
set_tests_properties(pipeline_v1_replay PROPERTIES TIMEOUT 5)

add_executable(encoder_v2_tests test_encoder_v2.c ../../Motor/encoder_tracker.c)
target_include_directories(encoder_v2_tests PRIVATE ../../Motor)
target_compile_features(encoder_v2_tests PRIVATE c_std_11)
if(MSVC)
    target_compile_options(encoder_v2_tests PRIVATE /W4)
else()
    target_compile_options(encoder_v2_tests PRIVATE -Wall -Wextra -Werror)
endif()
add_test(NAME encoder_v2_bounded COMMAND encoder_v2_tests)
set_tests_properties(encoder_v2_bounded PROPERTIES TIMEOUT 5)

foreach(mode 0 1)
    add_executable(safety_tests_${mode} test_safety.c ../../App/app_no_power.c
        ../../BSP/bsp_adc_safe.c ../../BSP/bsp_encoder_safe.c ../../BSP/bsp_safety.c
        ../../BSP/bsp_motor_enable.c
        ../../BSP/bsp_pwm.c ../../BSP/bsp_timer.c ../../Motor/encoder_tracker.c
        ../../Motor/motor_angle_speed.c ../../Control/foc_pipeline.c
        ../../Control/foc_transforms.c ../../Control/foc_pi.c ../../Control/foc_svpwm.c)
    target_include_directories(safety_tests_${mode} PRIVATE mock ../../BSP ../../App ../../Motor ../../Control)
    target_compile_definitions(safety_tests_${mode} PRIVATE DMF407_NO_POWER_CONTROL=${mode})
    target_compile_features(safety_tests_${mode} PRIVATE c_std_11)
    if(MSVC)
        target_compile_options(safety_tests_${mode} PRIVATE /W4)
    else()
        target_compile_options(safety_tests_${mode} PRIVATE -Wall -Wextra -Werror)
        target_link_libraries(safety_tests_${mode} PRIVATE m)
    endif()
    add_test(NAME app_mode_${mode} COMMAND safety_tests_${mode} app_replay)
    set_tests_properties(app_mode_${mode} PROPERTIES TIMEOUT 5)
endforeach()
foreach(group pwm_start pwm_stop adc_start adc_poll adc_bad svpwm_bad app_failure)
    if(group STREQUAL "adc_start")
        set(last 2)
    elseif(group STREQUAL "app_failure")
        set(last 3)
    elseif(group STREQUAL "adc_bad")
        set(last 3)
    elseif(group STREQUAL "svpwm_bad")
        set(last 7)
    else()
        set(last 5)
    endif()
    foreach(index RANGE 0 ${last})
        # ADC polling covers HAL_ERROR, HAL_BUSY and HAL_TIMEOUT separately.
        if(group STREQUAL "adc_poll")
            set(statuses 1 2 3)
        else()
            set(statuses 1)
        endif()
        foreach(status IN LISTS statuses)
            add_test(NAME v2_${group}_${index}_${status} COMMAND safety_tests_1 ${group} ${index} ${status})
            set_tests_properties(v2_${group}_${index}_${status} PROPERTIES TIMEOUT 5)
        endforeach()
    endforeach()
endforeach()
foreach(group encoder early_init valid)
    add_test(NAME v2_${group} COMMAND safety_tests_1 ${group})
    set_tests_properties(v2_${group} PROPERTIES TIMEOUT 5)
endforeach()
