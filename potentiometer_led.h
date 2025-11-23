#ifndef POTENTIOMETER_LED_H
#define POTENTIOMETER_LED_H

#include "pico/stdlib.h"

// --- ADC INPUT CONFIGURATION (Potentiometers) ---
// The Pico W has three available ADC channels (0, 1, 2) which map to specific GPIO pins.
#define POT_R_GPIO_PIN 26 // Maps to ADC0
#define POT_G_GPIO_PIN 27 // Maps to ADC1
#define POT_B_GPIO_PIN 28 // Maps to ADC2

// --- PWM OUTPUT CONFIGURATION (High-Intensity LEDs) ---
// Use GPIO pins that support PWM for the LEDs.
#define LED_R_GPIO_PIN 10
#define LED_G_GPIO_PIN 11
#define LED_B_GPIO_PIN 12

// The ADC provides 12-bit resolution (0 to 4095).
#define ADC_MAX_VALUE 4095

// The PWM counter is 16-bit, providing a higher resolution for smooth intensity control.
#define PWM_WRAP_VALUE 65535

/**
 * @brief Initializes the ADC for the three potentiometer pins and the PWM for the three LED pins.
 * Must be called once during system setup.
 */
void PotLED_Init(void);

/**
 * @brief Reads the raw ADC value from a specified potentiometer channel.
 * * @param gpio_pin The GPIO pin of the potentiometer (e.g., POT_R_GPIO_PIN).
 * @return uint16_t The raw ADC value (0-4095).
 */
uint16_t PotLED_ReadRaw(uint gpio_pin);

/**
 * @brief Reads the potentiometer value and updates the corresponding LED intensity using PWM.
 *
 * This function handles the full process: Read ADC -> Map to PWM -> Set LED brightness.
 *
 * @param pot_gpio_pin The GPIO pin of the potentiometer (ADC input).
 * @param led_gpio_pin The GPIO pin of the LED (PWM output).
 * @return uint16_t The raw ADC value read (0-4095).
 */
uint16_t PotLED_UpdateIntensity(uint pot_gpio_pin, uint led_gpio_pin);

#endif // POTENTIOMETER_LED_H