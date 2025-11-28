#ifndef COLOR_SENSOR_H
#define COLOR_SENSOR_H

#include <stdint.h>
#include <stdbool.h>
#include "pico/stdlib.h"

// --- FIXED PIN MAPPING ---
// Frequency scaling
#define TCS3200_S0_PIN 6
#define TCS3200_S1_PIN 7

// Color filter selection
#define TCS3200_S2_PIN 8
#define TCS3200_S3_PIN 9

// Output frequency pin
#define TCS3200_OUT_PIN 13

typedef enum
{
    TCS3200_SCALE_POWER_DOWN,
    TCS3200_SCALE_2_PERCENT,
    TCS3200_SCALE_20_PERCENT,
    TCS3200_SCALE_100_PERCENT
} tcs3200_scale_t;
typedef enum
{
    TCS3200_FILTER_RED,
    TCS3200_FILTER_BLUE,
    TCS3200_FILTER_CLEAR,
    TCS3200_FILTER_GREEN
} tcs3200_filter_t;

void TCS3200_Init(void);
void TCS3200_SetFrequencyScaling(tcs3200_scale_t scale);
void TCS3200_SetFilter(tcs3200_filter_t filter);
uint32_t TCS3200_ReadFrequencyHz(uint32_t gate_time_ms);
void TCS3200_ReadRGB(uint32_t gate_time_ms, uint32_t *r_hz, uint32_t *g_hz, uint32_t *b_hz);

#endif