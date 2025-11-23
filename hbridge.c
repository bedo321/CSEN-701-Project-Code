#include "hbridge.h"
#include "hardware/pwm.h"
#include "hardware/gpio.h"
#include <math.h> // Required for fabsf() to get the absolute value

// Enumeration for internal use only (not exposed in motor2.h)
typedef enum
{
    MOTOR_DIRECTION_FORWARD,
    MOTOR_DIRECTION_REVERSE,
    MOTOR_DIRECTION_BRAKE
} HBRIDGE_DIRECTION;

// Variable to store the specific PWM slice number for the chosen pin
static uint slice_num;

// --- INTERNAL H-BRIDGE CONTROL FUNCTIONS ---

/**
 * @brief Initializes the H-Bridge control pins (one PWM, two GPIO).
 */
static void HBridge_Init(void)
{
    // --- 1. Configure Direction Pins (GPIO Output) ---
    gpio_init(HBRIDGE_IN1_PIN);
    gpio_set_dir(HBRIDGE_IN1_PIN, GPIO_OUT);
    gpio_init(HBRIDGE_IN2_PIN);
    gpio_set_dir(HBRIDGE_IN2_PIN, GPIO_OUT);

    // Default to BRAKE (both low) initially for safety
    gpio_put(HBRIDGE_IN1_PIN, 0);
    gpio_put(HBRIDGE_IN2_PIN, 0);

    // --- 2. Configure PWM Pin (Speed Control) ---
    gpio_set_function(HBRIDGE_PWM_PIN, GPIO_FUNC_PWM);

    slice_num = pwm_gpio_to_slice_num(HBRIDGE_PWM_PIN);

    pwm_config config = pwm_get_default_config();
    pwm_config_set_wrap(&config, PWM_WRAP_VALUE);

    pwm_init(slice_num, &config, true); // Start PWM immediately
}

/**
 * @brief Sets the motor's direction by controlling the IN1 and IN2 pins.
 */
static void HBridge_SetDirection(HBRIDGE_DIRECTION direction)
{
    switch (direction)
    {
    case MOTOR_DIRECTION_FORWARD:
        // Set IN1 HIGH, IN2 LOW for Forward
        gpio_put(HBRIDGE_IN1_PIN, 1);
        gpio_put(HBRIDGE_IN2_PIN, 0);
        break;
    case MOTOR_DIRECTION_REVERSE:
        // Set IN1 LOW, IN2 HIGH for Reverse
        gpio_put(HBRIDGE_IN1_PIN, 0);
        gpio_put(HBRIDGE_IN2_PIN, 1);
        break;
    case MOTOR_DIRECTION_BRAKE:
        // Set both LOW for coast/stop.
        gpio_put(HBRIDGE_IN1_PIN, 0);
        gpio_put(HBRIDGE_IN2_PIN, 0);
        break;
    }
}

/**
 * @brief Sets the motor's speed using PWM duty cycle.
 */
static void HBridge_SetSpeed(uint16_t duty_cycle_level)
{
    // Clamp the value just in case
    if (duty_cycle_level > PWM_WRAP_VALUE)
    {
        duty_cycle_level = PWM_WRAP_VALUE;
    }

    // Apply the duty cycle to the PWM pin
    pwm_set_gpio_level(HBRIDGE_PWM_PIN, duty_cycle_level);
}

// --- PUBLIC MOTOR CONTROL FUNCTIONS ---

void Motor_Init(void)
{
    // Initialize the H-Bridge hardware
    HBridge_Init();

    // Start with motor speed at 0 and direction set
    HBridge_SetSpeed(0);
    HBridge_SetDirection(MOTOR_DIRECTION_FORWARD);
}

void Motor_UpdateActuation(float correctness_percent)
{
    uint16_t duty_cycle_level = 0;
    float absolute_percent;
    HBRIDGE_DIRECTION direction;

    // --- 1. Determine Direction and Absolute Speed ---
    if (correctness_percent > 0.0f)
    {
        direction = MOTOR_DIRECTION_FORWARD;
        absolute_percent = correctness_percent;
    }
    else if (correctness_percent < 0.0f)
    {
        direction = MOTOR_DIRECTION_REVERSE;
        absolute_percent = fabsf(correctness_percent);
    }
    else
    {
        direction = MOTOR_DIRECTION_BRAKE;
        absolute_percent = 0.0f;
    }

    // --- 2. Apply Direction ---
    // Note: Motor_Stop() logic will override the direction to BRAKE if power is 0.
    HBridge_SetDirection(direction);

    // --- 3. Validation Logic (Project Requirement using Absolute Value) ---

    // Clamp the absolute percentage to a maximum of 100.0
    if (absolute_percent > 100.0f)
    {
        absolute_percent = 100.0f;
    }

    // Rule 1: SUCCESS condition (>= 97%)
    if (absolute_percent >= 97.0f)
    {
        // The motor completes the full rotation (Max Power)
        duty_cycle_level = PWM_WRAP_VALUE;
    }
    // Rule 2: FAILURE/TUNING condition (< 97%)
    else
    {
        // Proportional Feedback:
        // Formula: (Percentage / 100) * Max_PWM_Value
        duty_cycle_level = (uint16_t)((absolute_percent / 100.0f) * PWM_WRAP_VALUE);
    }

    // --- 4. Apply Speed (and handle stop/brake) ---
    if (duty_cycle_level == 0)
    {
        // If speed is zero, ensure the motor is fully stopped/braked
        Motor_Stop();
    }
    else
    {
        HBridge_SetSpeed(duty_cycle_level);
    }
}

void Motor_Stop(void)
{
    // Set speed to 0 and direction to brake for immediate stop/coast
    HBridge_SetSpeed(0);
    HBridge_SetDirection(MOTOR_DIRECTION_BRAKE);
}