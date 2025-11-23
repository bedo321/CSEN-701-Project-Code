#include "potentiometer_led.h"
#include "hardware/adc.h"
#include "hardware/pwm.h"
#include <math.h> // Though not strictly needed for this basic mapping, good practice for embedded logic

// Helper function to convert a GPIO pin to its corresponding ADC channel index (0-2)
static uint gpio_to_adc_channel(uint gpio_pin)
{
    // ADC0 is GPIO 26, ADC1 is GPIO 27, ADC2 is GPIO 28
    return gpio_pin - 26;
}

// Helper function to map a PWM pin to its slice number
static uint get_pwm_slice_num(uint gpio_pin)
{
    return pwm_gpio_to_slice_num(gpio_pin);
}

void PotLED_Init(void)
{
    // --- 1. ADC Initialization ---
    adc_init();

    // Initialize the three ADC-capable GPIO pins
    adc_gpio_init(POT_R_GPIO_PIN);
    adc_gpio_init(POT_G_GPIO_PIN);
    adc_gpio_init(POT_B_GPIO_PIN);

    // Select the first channel (ADC0 / POT_R) for the initial read state
    adc_select_input(gpio_to_adc_channel(POT_R_GPIO_PIN));

    // --- 2. PWM Initialization ---

    // Define the PWM pins
    uint led_pins[] = {LED_R_GPIO_PIN, LED_G_GPIO_PIN, LED_B_GPIO_PIN};

    for (int i = 0; i < 3; i++)
    {
        uint pin = led_pins[i];

        // Set the GPIO function to PWM
        gpio_set_function(pin, GPIO_FUNC_PWM);

        // Get the slice number for this pin
        uint slice = get_pwm_slice_num(pin);

        // Get the default configuration
        pwm_config config = pwm_get_default_config();

        // Set the wrap (resolution) to 16-bit max
        pwm_config_set_wrap(&config, PWM_WRAP_VALUE);

        // Initialize the PWM slice with the config and start it
        pwm_init(slice, &config, true);

        // Start LEDs at 0 intensity (off)
        pwm_set_gpio_level(pin, 0);
    }
}

uint16_t PotLED_ReadRaw(uint gpio_pin)
{
    // Select the correct ADC input channel for the given GPIO pin
    adc_select_input(gpio_to_adc_channel(gpio_pin));

    // Read the 12-bit value (0-4095)
    return adc_read();
}

uint16_t PotLED_UpdateIntensity(uint pot_gpio_pin, uint led_gpio_pin)
{

    // 1. Read the raw ADC value (0 - 4095)
    uint16_t raw_adc = PotLED_ReadRaw(pot_gpio_pin);

    // 2. Map the 12-bit ADC value (0-4095) to the 16-bit PWM range (0-65535)
    // Formula: output = (input / ADC_MAX_VALUE) * PWM_WRAP_VALUE
    // We use 64 as the scale factor because 65535 / 4095 is approximately 16.
    // A slightly more precise calculation:
    uint32_t pwm_level = (raw_adc * (PWM_WRAP_VALUE + 1)) / (ADC_MAX_VALUE + 1);

    // 3. Set the PWM duty cycle for the LED
    uint slice = get_pwm_slice_num(led_gpio_pin);

    // Apply the new duty cycle
    pwm_set_chan_level(slice, pwm_gpio_to_channel(led_gpio_pin), (uint16_t)pwm_level);

    // 4. Return the raw ADC value for external use (like displaying on LCD or sending via Wi-Fi)
    return raw_adc;
}