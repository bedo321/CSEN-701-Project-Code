#ifndef COLOR_SENSOR_H
#define COLOR_SENSOR_H

// Standard integer types
#include <stdint.h>
#include <stdbool.h>

// Pico SDK stdlib
#include "pico/stdlib.h"

// -----------------------------------------------------------------------------
// Hardware configuration (Matched to your Main.c wiring)
// -----------------------------------------------------------------------------

// Frequency scaling pins S0 & S1
#ifndef TCS3200_S0_PIN
#define TCS3200_S0_PIN 2
#endif

#ifndef TCS3200_S1_PIN
#define TCS3200_S1_PIN 3
#endif

// Color filter selection pins S2 & S3
#ifndef TCS3200_S2_PIN
#define TCS3200_S2_PIN 4
#endif

#ifndef TCS3200_S3_PIN
#define TCS3200_S3_PIN 5
#endif

// Output frequency pin (fo)
#ifndef TCS3200_OUT_PIN
#define TCS3200_OUT_PIN 6
#endif

// NOTE: OE Pin is removed because your module hardwires it to Enabled.

// -----------------------------------------------------------------------------
// Types
// -----------------------------------------------------------------------------

typedef enum
{
    TCS3200_SCALE_POWER_DOWN = 0,
    TCS3200_SCALE_2_PERCENT,
    TCS3200_SCALE_20_PERCENT,
    TCS3200_SCALE_100_PERCENT
} tcs3200_scale_t;

typedef enum
{
    TCS3200_FILTER_RED = 0,
    TCS3200_FILTER_BLUE,
    TCS3200_FILTER_CLEAR,
    TCS3200_FILTER_GREEN
} tcs3200_filter_t;

// -----------------------------------------------------------------------------
// Public API
// -----------------------------------------------------------------------------

/**
 * @brief Initialize the TCS3200 GPIOs and default configuration.
 * Sets S0-S3 as Outputs and OUT as Input.
 */
void TCS3200_Init(void);

/**
 * @brief Set the output frequency scaling using S0/S1.
 */
void TCS3200_SetFrequencyScaling(tcs3200_scale_t scale);

/**
 * @brief Select which color filter is active.
 */
void TCS3200_SetFilter(tcs3200_filter_t filter);

/**
 * @brief Measure the output frequency (Hz) over a fixed gate time.
 */
uint32_t TCS3200_ReadFrequencyHz(uint32_t gate_time_ms);

/**
 * @brief Measure red, green, and blue frequencies sequentially.
 */
void TCS3200_ReadRGB(uint32_t gate_time_ms,
                     uint32_t *r_hz,
                     uint32_t *g_hz,
                     uint32_t *b_hz);

#endif