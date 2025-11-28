# Set minimum required version of CMake
cmake_minimum_required(VERSION 3.12)

# --- CRITICAL: Set Board to Pico W ---
set(PICO_BOARD pico_w)

# Include build functions from Pico SDK
include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

# Set name of project
project(milestone3 C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Initialize SDK
pico_sdk_init()

# Define Executable and Source Files
add_executable(${PROJECT_NAME} 
    main.c
    lcd.c
    button.c
    potentiometer_led.c
    hbridge.c
    color_sensor.c
    wifi_server.c
)

# --- CRITICAL FIX IS HERE ---
# This tells the compiler to look in the current folder for lwipopts.h
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
# ----------------------------

# Create output files
pico_add_extra_outputs(${PROJECT_NAME})

# Link Libraries
target_link_libraries(${PROJECT_NAME} 
    pico_stdlib
    pico_cyw43_arch_lwip_threadsafe_background
    hardware_adc
    hardware_pwm
    hardware_i2c
)

# Define Wi-Fi Credentials
target_compile_definitions(${PROJECT_NAME}
    PRIVATE
        CYW43_LWIP=1
        WIFI_SSID="PICO_W_Color_Sensor" 
        WIFI_PASSWORD="ahmed"
)

# Enable USB Output
pico_enable_stdio_usb(${PROJECT_NAME} 1)
pico_enable_stdio_uart(${PROJECT_NAME} 0)