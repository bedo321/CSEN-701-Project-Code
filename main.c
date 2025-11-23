#include <stdio.h>
#include "pico/stdlib.h"
#include "lcd.h"               // The driver from the previous step
#include "button.h"            // The new button driver
#include "potentiometer_led.h" // The potentiometer & LED driver
#include "color_sensor.h"      // The color sensor driver
#include "hbridge.h"           // The H-Bridge motor driver
#define UPDATE_DELAY_MS 100
#define SENSOR_GATE_TIME_MS 20
#define MIN_MOTOR_START_SPEED 100.0f // We set the motor speed to 100% now because our current battery can't supply much voltage

// ** PART 1 OF THE VIDEO: LCD Display with Button Control **
// This application initializes the LCD and a button. When the button
// is pressed, it displays a mock IP address on the LCD. When released,
// it returns to the menu.

// int main()
// {
//     stdio_init_all();

//     // 1. Initialize Hardware
//     lcd_init();
//     button_init();

//     // 2. Define states to prevent screen flickering
//     // We only want to write to the LCD when the state CHANGES.
//     bool showing_ip = false;

//     // 3. Initial Screen Setup
//     lcd_clear();
//     lcd_set_cursor(0, 0);
//     lcd_string("Status: READY");
//     lcd_set_cursor(1, 0);
//     lcd_string("Press Button...");

//     while (1)
//     {
//         if (button_is_pressed())
//         {
//             // Button is being held down
//             if (!showing_ip)
//             {
//                 // Only update the screen if we haven't already!
//                 lcd_clear();
//                 lcd_set_cursor(0, 0);
//                 lcd_string("IP Address:");
//                 lcd_set_cursor(1, 0);
//                 lcd_string("192.168.1.1");

//                 showing_ip = true; // Remember that we are showing IP
//             }
//         }
//         else
//         {
//             // Button is released
//             if (showing_ip)
//             {
//                 // We were showing IP, but button was let go. Return to menu.
//                 lcd_clear();
//                 lcd_set_cursor(0, 0);
//                 lcd_string("Status: READY");
//                 lcd_set_cursor(1, 0);
//                 lcd_string("Press Button...");

//                 showing_ip = false; // Remember we are back to default
//             }
//         }

//         // Small delay to debounce and save CPU
//         sleep_ms(50);
//     }

//     return 0;
// }

// ** PART 2 OF THE VIDEO: Potentiometer & LED reading **
// int main()
// {
//     // Initialize standard I/O (USB CDC) for printing to the terminal
//     stdio_init_all();

//     // --- Subsystem Initialization ---
//     // 1. Initialize the Potentiometer/LED (Spectral Flow Interface)
//     PotLED_Init();

//     printf("System Ready.\n");
//     printf("--- Spectral Flow Interface Active ---\n");
//     printf("Turn Dials (GPIO 26, 27, 28) to adjust LED brightness (GPIO 10, 11, 12).\n");
//     printf("R   | G   | B   \n");
//     printf("-------------------\n");

//     uint16_t r_raw, g_raw, b_raw;

//     while (true)
//     {
//         // --- Spectral Flow Interface (Potentiometer -> LED) ---
//         // Read the pot, map the value to PWM, and set the LED intensity.
//         r_raw = PotLED_UpdateIntensity(POT_R_GPIO_PIN, LED_B_GPIO_PIN);
//         g_raw = PotLED_UpdateIntensity(POT_G_GPIO_PIN, LED_G_GPIO_PIN);
//         b_raw = PotLED_UpdateIntensity(POT_B_GPIO_PIN, LED_R_GPIO_PIN);

//         // Print raw values to the console/LCD display (0-4095)
//         // The \r resets the cursor, creating a live, single-line display.
//         printf("\r%4d|%4d|%4d", r_raw, g_raw, b_raw);
//         fflush(stdout); // Force print immediately

//         // Short delay to avoid reading the ADC/printing too fast
//         sleep_ms(10);
//     }
// }

// ** PART 3 OF THE VIDEO: Color Sensor & H-Bridge Motor Control **

/**
 * @brief Calculates how "Red" the object is as a percentage (0-100).
 * * Logic: In a perfect red object, the Red frequency is high,
 * and Green/Blue are low. We calculate the ratio of Red to Total Light.
 */
float calculate_red_dominance(uint32_t r, uint32_t g, uint32_t b)
{
    uint32_t total_light = r + g + b;

    // Prevent division by zero if it's pitch black
    if (total_light == 0)
    {
        return 0.0f;
    }

    // Calculate Red percentage
    // Example: R=800, G=100, B=100 -> Total=1000 -> Red is 80%
    float red_ratio = (float)r / (float)total_light;

    return red_ratio * 100.0f;
}

int main()
{
    // 1. Initialize Standard I/O
    stdio_init_all();
    sleep_ms(2000); // Wait for USB
    printf("System Init - Beep Fix Applied...\n");

    // 2. Initialize Subsystems
    Motor_Init();
    TCS3200_Init();

    // Configure Color Sensor
    TCS3200_SetFrequencyScaling(TCS3200_SCALE_20_PERCENT);

    printf("Active. Waiting for RED > 50%%\n");

    uint32_t r, g, b;
    float red_accuracy = 0.0f;
    float motor_input = 0.0f;

    while (true)
    {
        // --- STEP 1: Read Color Sensor ---
        TCS3200_ReadRGB(SENSOR_GATE_TIME_MS, &r, &g, &b);

        // --- STEP 2: Process Data ---
        red_accuracy = calculate_red_dominance(r, g, b);

        // --- STEP 3: Motor Logic (The Fix) ---

        // Check if we met the threshold
        if (red_accuracy > 50.0f)
        {

            motor_input = red_accuracy;

            // Hard cap at 100% to prevent errors
            if (motor_input > 100.0f)
                motor_input = 100.0f;

            // Safety check: Ensure we are actually sending enough power to move
            if (motor_input < MIN_MOTOR_START_SPEED)
            {
                motor_input = MIN_MOTOR_START_SPEED;
            }
        }
        else
        {
            // Not red enough? Full stop.
            motor_input = 0.0f;
        }

        // Apply to motor driver
        Motor_UpdateActuation(motor_input);

        // --- STEP 4: Debug Output ---
        printf("\rR:%4u G:%4u B:%4u | Red Acc: %5.1f%% | Motor Power: %5.1f%%",
               r, g, b, red_accuracy, motor_input);

        sleep_ms(UPDATE_DELAY_MS);
    }

    return 0;
}