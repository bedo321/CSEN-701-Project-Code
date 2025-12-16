#include <stdio.h>
#include <math.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "lcd.h"
#include "button.h"
#include "potentiometer_led.h"
#include "hbridge.h"
#include "color_sensor.h"
#include "wifi_server.h"

// --- GEOMETRIC SEQUENCE REWARD ---
/**
 * @brief Display geometric sequence reward when success threshold reached
 * Sequence: 2, 6, 18, 54, X (ratio = 3)
 */

// --- CONFIGURATION ---
// Target RGB values (The "Treasure" color)
#define TARGET_R_HZ 1200
#define TARGET_G_HZ 1000
#define TARGET_B_HZ 1600
// Max frequency expected (for normalization)
#define MAX_EXPECTED_HZ 5000.0f

// Helper to calculate correctness % based on Euclidean Distance
float calculate_correctness(uint32_t r, uint32_t g, uint32_t b)
{
    // 1. Avoid division by zero
    if (TARGET_R_HZ == 0 || TARGET_G_HZ == 0 || TARGET_B_HZ == 0)
        return 0.0f;

    // 2. Calculate % difference for each channel individually
    // A value of 0.0 means perfect. 1.0 means 100% wrong.
    float diff_r = fabsf((float)r - TARGET_R_HZ) / TARGET_R_HZ;
    float diff_g = fabsf((float)g - TARGET_G_HZ) / TARGET_G_HZ;
    float diff_b = fabsf((float)b - TARGET_B_HZ) / TARGET_B_HZ;

    // 3. Average the errors
    float total_error = (diff_r + diff_g + diff_b) / 3.0f;

    // 4. Invert: If error is 0, accuracy is 100%.
    // If error is > 1.0 (more than 100% off), accuracy is 0.
    float accuracy = 100.0f * (1.0f - total_error);

    // 5. Clamp results
    if (accuracy < 0.0f)
        accuracy = 0.0f;
    if (accuracy > 100.0f)
        accuracy = 100.0f;

    return accuracy;
}

int main()
{
    stdio_init_all();

    // 1. Initialize Subsystems
    lcd_init();
    lcd_string("Booting...");

    button_init();
    PotLED_Init();
    Motor_Init();
    TCS3200_Init();

    // 2. Initialize Wi-Fi (AP Mode)
    wifi_init_ap("Treasure_Hunt", "password123");

    // Update LCD with Status
    lcd_clear();
    lcd_string("IP: 192.168.4.1");
    lcd_set_cursor(1, 0);
    lcd_string("Waiting Start...");

    // Wait for button press to start the game
    while (!button_is_pressed())
    {
        wifi_poll();
        sleep_ms(50);
    }

    lcd_clear();
    lcd_string("Game Active!");

    // Variables for the loop
    uint32_t sensor_r = 0, sensor_g = 0, sensor_b = 0;
    uint16_t pot_r = 0, pot_g = 0, pot_b = 0;
    float correctness = 0.0f;
    bool success_reward_shown = false;

    while (true)
    {
        // Poll WiFi
        wifi_poll();

        // Read potentiometers and update LEDs
        pot_r = PotLED_UpdateIntensity(POT_R_GPIO_PIN, LED_R_GPIO_PIN);
        pot_g = PotLED_UpdateIntensity(POT_G_GPIO_PIN, LED_G_GPIO_PIN);
        pot_b = PotLED_UpdateIntensity(POT_B_GPIO_PIN, LED_B_GPIO_PIN);

        // Read color sensor
        TCS3200_ReadRGB(10, &sensor_r, &sensor_g, &sensor_b);

        // Calculate correctness
        correctness = calculate_correctness(sensor_r, sensor_g, sensor_b);

        // Control motor
        Motor_UpdateActuation(correctness);

        // Display success message
        if (correctness >= 97.0f && !success_reward_shown)
        {
            success_reward_shown = true;
            lcd_clear();
            lcd_string("SUCCESS! Seq:");
            lcd_set_cursor(1, 0);
            lcd_string("2,6,18,54,X");
        }

        // Update web server
        wifi_update_data((uint16_t)sensor_r, (uint16_t)sensor_g, (uint16_t)sensor_b, correctness);

        // Update LCD
        static int lcd_counter = 0;
        if (lcd_counter++ > 10 && !success_reward_shown)
        {
            lcd_set_cursor(1, 0);
            char buf[16];
            snprintf(buf, 16, "Acc: %3.1f%%", correctness);
            lcd_string(buf);
            lcd_counter = 0;
        }

        sleep_ms(50);
    }
    return 0;
}