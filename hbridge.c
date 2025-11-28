#include "hbridge.h"
#include "hardware/pwm.h"
#include "hardware/gpio.h"
#include <math.h>

// Enumeration for internal use
typedef enum
{
    MOTOR_DIRECTION_FORWARD,
    MOTOR_DIRECTION_REVERSE,
    MOTOR_DIRECTION_BRAKE
} HBRIDGE_DIRECTION;

static uint slice_num;

// --- INTERNAL HELPERS ---

static void HBridge_Init(void)
{
    // Configure Direction Pins
    gpio_init(HBRIDGE_IN1_PIN);
    gpio_set_dir(HBRIDGE_IN1_PIN, GPIO_OUT);
    gpio_init(HBRIDGE_IN2_PIN);
    gpio_set_dir(HBRIDGE_IN2_PIN, GPIO_OUT);

    // Default to BRAKE
    gpio_put(HBRIDGE_IN1_PIN, 0);
    gpio_put(HBRIDGE_IN2_PIN, 0);

    // Configure PWM Pin
    gpio_set_function(HBRIDGE_PWM_PIN, GPIO_FUNC_PWM);
    slice_num = pwm_gpio_to_slice_num(HBRIDGE_PWM_PIN);
    pwm_config config = pwm_get_default_config();
    pwm_config_set_wrap(&config, PWM_WRAP_VALUE);
    pwm_init(slice_num, &config, true);
}

static void HBridge_SetDirection(HBRIDGE_DIRECTION direction)
{
    switch (direction)
    {
    case MOTOR_DIRECTION_FORWARD:
        gpio_put(HBRIDGE_IN1_PIN, 1);
        gpio_put(HBRIDGE_IN2_PIN, 0);
        break;
    case MOTOR_DIRECTION_REVERSE:
        gpio_put(HBRIDGE_IN1_PIN, 0);
        gpio_put(HBRIDGE_IN2_PIN, 1);
        break;
    case MOTOR_DIRECTION_BRAKE:
        gpio_put(HBRIDGE_IN1_PIN, 0);
        gpio_put(HBRIDGE_IN2_PIN, 0);
        break;
    }
}

static void HBridge_SetSpeed(uint16_t duty_cycle_level)
{
    pwm_set_gpio_level(HBRIDGE_PWM_PIN, duty_cycle_level);
}

// --- PUBLIC FUNCTIONS ---

void Motor_Init(void)
{
    HBridge_Init();
    Motor_Stop();
}

void Motor_UpdateActuation(float correctness_percent)
{
    // --- THE FIX ---
    // Only spin if the user has reached the "Treasure" state (> 97%)

    if (correctness_percent >= 97.0f)
    {
        // UNLOCKED: Spin the motor forward at full speed
        HBridge_SetDirection(MOTOR_DIRECTION_FORWARD);
        HBridge_SetSpeed(PWM_WRAP_VALUE);
    }
    else
    {
        // LOCKED: Keep the motor stopped
        Motor_Stop();
    }
}

void Motor_Stop(void)
{
    HBridge_SetSpeed(0);
    HBridge_SetDirection(MOTOR_DIRECTION_BRAKE);
}