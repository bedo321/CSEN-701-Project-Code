#ifndef hbridge
#define hbirdge

#include "pico/stdlib.h"

// --- H-Bridge Pin Configuration ---
// These pins are designed for a common driver like the L298N or similar
// that uses two digital inputs (IN1/IN2) and one PWM input (ENA).
#define HBRIDGE_PWM_PIN 16 // The speed control pin (Connect to ENA on L298N)
#define HBRIDGE_IN1_PIN 17 // Direction Input 1 (Connect to IN1)
#define HBRIDGE_IN2_PIN 18 // Direction Input 2 (Connect to IN2)

// --- PWM CONFIGURATION ---
// The maximum value for the PWM counter (100% duty cycle)
#define PWM_WRAP_VALUE 65535

/**
 * @brief Initializes the DC Motor GPIO and H-Bridge hardware.
 * Must be called once during system setup.
 */
void Motor_Init(void);

/**
 * @brief Updates the motor state based on the Chromatic Validation percentage.
 * * * This function supports bidirectional control:
 * 1. If Correctness is POSITIVE (+0.1 to +100.0): Motor runs FORWARD.
 * 2. If Correctness is NEGATIVE (-0.1 to -100.0): Motor runs REVERSE.
 * 3. If the absolute value is >= 97%: Full power is applied.
 * * @param correctness_percent A float value from -100.0 to 100.0 representing the control effort.
 */
void Motor_UpdateActuation(float correctness_percent);

/**
 * @brief Immediately stops the motor (Safety/Shutdown).
 */
void Motor_Stop(void);

#endif // MOTOR2_H