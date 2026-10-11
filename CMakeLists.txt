cmake_minimum_required(VERSION 3.22)

#
# This file is generated only once,
# and is not re-generated if converter is called multiple times.
#
# User is free to modify the file as much as necessary
#

# Setup compiler settings
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)


# Define the build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

# Set the project name
set(CMAKE_PROJECT_NAME DMF407_FOC_HAL)

# Include toolchain file
include("cmake/gcc-arm-none-eabi.cmake")

# Enable compile command to ease indexing with e.g. clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# Core project settings
project(${CMAKE_PROJECT_NAME} LANGUAGES C ASM)
message("Build type: " ${CMAKE_BUILD_TYPE})

# Create an executable object type
add_executable(${CMAKE_PROJECT_NAME})

# Add STM32CubeMX generated sources
include("cmake/check_dependencies.cmake")
add_subdirectory(cmake/stm32cubemx)

# Link directories setup
target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined library search paths
)

# Add sources to executable
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user sources here
    ${CMAKE_CURRENT_SOURCE_DIR}/BSP/bsp_pwm.c
    ${CMAKE_CURRENT_SOURCE_DIR}/BSP/bsp_adc.c
    ${CMAKE_CURRENT_SOURCE_DIR}/BSP/bsp_encoder.c
    ${CMAKE_CURRENT_SOURCE_DIR}/BSP/bsp_timer.c
    ${CMAKE_CURRENT_SOURCE_DIR}/BSP/bsp_timing.c
    ${CMAKE_CURRENT_SOURCE_DIR}/BSP/bsp_motor_enable.c
    ${CMAKE_CURRENT_SOURCE_DIR}/BSP/bsp_uart.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Motor/motor_angle_speed.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Control/foc_transforms.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Control/foc_pi.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Control/foc_svpwm.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Control/foc_pipeline.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Motor/encoder_tracker.c
    ${CMAKE_CURRENT_SOURCE_DIR}/BSP/bsp_encoder_safe.c
    ${CMAKE_CURRENT_SOURCE_DIR}/BSP/bsp_safety.c
    ${CMAKE_CURRENT_SOURCE_DIR}/BSP/bsp_adc_safe.c
    ${CMAKE_CURRENT_SOURCE_DIR}/App/app_no_power.c
    ${CMAKE_CURRENT_SOURCE_DIR}/App/app_motion_sim.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Motion/motion_profile.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Motion/motion.c
)

# Add include paths
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined include paths
    ${CMAKE_CURRENT_SOURCE_DIR}/BSP
    ${CMAKE_CURRENT_SOURCE_DIR}/Motor
    ${CMAKE_CURRENT_SOURCE_DIR}/Control
    ${CMAKE_CURRENT_SOURCE_DIR}/App
    ${CMAKE_CURRENT_SOURCE_DIR}/Motion
)

# Add project symbols (macros)
option(DMF407_NO_POWER_CONTROL "Run FOC replay on MCU with PF10 forced Low (L4 timing unverified)" OFF)
option(DMF407_DWT_DIAGNOSTICS "Measure TIM1 IRQ C body cycles for no-power L4 diagnostics" OFF)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined symbols
    DMF407_NO_POWER_CONTROL=$<BOOL:${DMF407_NO_POWER_CONTROL}>
    DMF407_DWT_DIAGNOSTICS=$<BOOL:${DMF407_DWT_DIAGNOSTICS}>
)

# Add linked libraries
target_link_libraries(${CMAKE_PROJECT_NAME}
    stm32cubemx

    # Add user defined libraries
)

# 生成 .bin / .hex 固件镜像
# 任务书 §13「构建纪律」要求构建产物包含 ELF / MAP / BIN
add_custom_command(TARGET ${CMAKE_PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${CMAKE_PROJECT_NAME}> ${CMAKE_PROJECT_NAME}.bin
    COMMAND ${CMAKE_OBJCOPY} -O ihex   $<TARGET_FILE:${CMAKE_PROJECT_NAME}> ${CMAKE_PROJECT_NAME}.hex
    COMMENT "Generating ${CMAKE_PROJECT_NAME}.bin / ${CMAKE_PROJECT_NAME}.hex"
    VERBATIM
)
