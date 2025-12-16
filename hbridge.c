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
static bool motor_locked = false; // Lock motor once 97% success reached

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
    // If motor locked (already completed success), don't update
    if (motor_locked)
    {
        return;
    }

    // --- SIMPLE PROPORTIONAL MOTOR CONTROL ---
    // Motor position follows correctness % directly:
    // 0% correctness = motor at 0% position
    // 97%+ correctness = motor performs full 360° rotation and locks
    // Motor rotates smoothly to match the current percentage

    const uint32_t FULL_ROTATION_TIME_MS = 2000; // Time for full 360° rotation
    const float MIN_CHANGE_THRESHOLD = 0.5f;     // Ignore tiny changes

    static float current_position = 0.0f;   // Current motor position (0-100%)
    static uint32_t rotation_start_ms = 0;  // When current rotation began
    static uint32_t target_duration_ms = 0; // How long this rotation should take
    static HBRIDGE_DIRECTION direction = MOTOR_DIRECTION_BRAKE;
    static bool rotating = false;                 // Currently moving?
    static bool success_rotation_started = false; // Track if success rotation began

    uint32_t now_ms = to_ms_since_boot(get_absolute_time());

    // At 97%+, perform full 360° rotation once then lock
    if (correctness_percent >= 97.0f)
    {
        if (!success_rotation_started)
        {
            // First time hitting 97%: start the full rotation
            direction = MOTOR_DIRECTION_FORWARD;
            target_duration_ms = FULL_ROTATION_TIME_MS;
            rotation_start_ms = now_ms;
            rotating = true;
            success_rotation_started = true;
            HBridge_SetDirection(direction);
            HBridge_SetSpeed((uint16_t)(PWM_WRAP_VALUE * 0.80f));
        }
        else if (rotating)
        {
            // Check if rotation complete
            uint32_t elapsed = now_ms - rotation_start_ms;
            if (elapsed >= target_duration_ms)
            {
                Motor_Stop();
                rotating = false;
                current_position = 100.0f;
                motor_locked = true; // Lock motor - no more updates ever
            }
        }
        return;
    }

    // Below 97%: smooth proportional movement
    float target_position = correctness_percent; // 0-97%
    float position_diff = target_position - current_position;

    // Check if position has changed meaningfully
    if (fabsf(position_diff) < MIN_CHANGE_THRESHOLD)
    {
        // Tiny change, ignore
        if (rotating)
        {
            // Check if current rotation complete
            uint32_t elapsed = now_ms - rotation_start_ms;
            if (elapsed >= target_duration_ms)
            {
                Motor_Stop();
                rotating = false;
                current_position = target_position;
            }
        }
        return;
    }

    if (!rotating)
    {
        // Start new rotation to target position
        direction = (position_diff > 0) ? MOTOR_DIRECTION_FORWARD : MOTOR_DIRECTION_REVERSE;
        float movement_ratio = fabsf(position_diff) / 100.0f; // What % of full rotation?
        target_duration_ms = (uint32_t)(movement_ratio * FULL_ROTATION_TIME_MS);
        rotation_start_ms = now_ms;
        rotating = true;

        HBridge_SetDirection(direction);
        HBridge_SetSpeed((uint16_t)(PWM_WRAP_VALUE * 0.80f));
    }
    else
    {
        // Check if current rotation complete
        uint32_t elapsed = now_ms - rotation_start_ms;
        if (elapsed >= target_duration_ms)
        {
            Motor_Stop();
            rotating = false;
            current_position = target_position;
        }
    }
}

void Motor_Stop(void)
{
    HBridge_SetSpeed(0);
    HBridge_SetDirection(MOTOR_DIRECTION_BRAKE);
}