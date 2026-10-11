if(NOT DEFINED PROJECT_ROOT)
    get_filename_component(PROJECT_ROOT "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)
else()
    get_filename_component(PROJECT_ROOT "${PROJECT_ROOT}" ABSOLUTE)
endif()
file(READ "${PROJECT_ROOT}/cmake/stm32cubemx/CMakeLists.txt" cube_sources)
string(REGEX MATCHALL "[.][.]/[.][.]/Drivers/[A-Za-z0-9_/]+[.]c" driver_sources "${cube_sources}")
set(required
    Drivers/STM32F4xx_HAL_Driver/Inc/stm32f4xx_hal.h
    Drivers/CMSIS/Device/ST/STM32F4xx/Include/stm32f407xx.h
    Drivers/CMSIS/Device/ST/STM32F4xx/Include/system_stm32f4xx.h
    Drivers/CMSIS/Include/core_cm4.h
    Drivers/CMSIS/Include/cmsis_gcc.h
)
foreach(source IN LISTS driver_sources)
    string(REPLACE "../../" "" source "${source}")
    list(APPEND required "${source}")
endforeach()
set(missing "")
foreach(source IN LISTS required)
    if(NOT EXISTS "${PROJECT_ROOT}/${source}")
        string(APPEND missing "\n  ${source}")
    endif()
endforeach()
if(missing)
    message(FATAL_ERROR "Missing HAL/CMSIS:${missing}\nRestore Drivers from STM32Cube FW_F4 V1.28.3; see README.md. Host tests do not require Drivers.")
endif()
message(STATUS "HAL/CMSIS dependency files present (version provenance must be checked against FW_F4 V1.28.3).")
