#include "color_sensor.h"

// -----------------------------------------------------------------------------
// Internal helpers
// -----------------------------------------------------------------------------

static inline void tcs3200_set_s0_s1(tcs3200_scale_t scale)
{
    switch (scale)
    {
    default:
    case TCS3200_SCALE_POWER_DOWN:
        gpio_put(TCS3200_S0_PIN, 0);
        gpio_put(TCS3200_S1_PIN, 0);
        break;

    case TCS3200_SCALE_2_PERCENT:
        gpio_put(TCS3200_S0_PIN, 0);
        gpio_put(TCS3200_S1_PIN, 1);
        break;

    case TCS3200_SCALE_20_PERCENT:
        gpio_put(TCS3200_S0_PIN, 1);
        gpio_put(TCS3200_S1_PIN, 0);
        break;

    case TCS3200_SCALE_100_PERCENT:
        gpio_put(TCS3200_S0_PIN, 1);
        gpio_put(TCS3200_S1_PIN, 1);
        break;
    }
}

static inline void tcs3200_set_s2_s3(tcs3200_filter_t filter)
{
    switch (filter)
    {
    default:
    case TCS3200_FILTER_RED:
        gpio_put(TCS3200_S2_PIN, 0);
        gpio_put(TCS3200_S3_PIN, 0);
        break;

    case TCS3200_FILTER_BLUE:
        gpio_put(TCS3200_S2_PIN, 0);
        gpio_put(TCS3200_S3_PIN, 1);
        break;

    case TCS3200_FILTER_CLEAR:
        gpio_put(TCS3200_S2_PIN, 1);
        gpio_put(TCS3200_S3_PIN, 0);
        break;

    case TCS3200_FILTER_GREEN:
        gpio_put(TCS3200_S2_PIN, 1);
        gpio_put(TCS3200_S3_PIN, 1);
        break;
    }
}

// -----------------------------------------------------------------------------
// Public API implementation
// -----------------------------------------------------------------------------

void TCS3200_Init(void)
{
    // Configure control pins as outputs
    gpio_init(TCS3200_S0_PIN);
    gpio_set_dir(TCS3200_S0_PIN, GPIO_OUT);

    gpio_init(TCS3200_S1_PIN);
    gpio_set_dir(TCS3200_S1_PIN, GPIO_OUT);

    gpio_init(TCS3200_S2_PIN);
    gpio_set_dir(TCS3200_S2_PIN, GPIO_OUT);

    gpio_init(TCS3200_S3_PIN);
    gpio_set_dir(TCS3200_S3_PIN, GPIO_OUT);

    // Configure OUT pin as input for frequency measurement
    gpio_init(TCS3200_OUT_PIN);
    gpio_set_dir(TCS3200_OUT_PIN, GPIO_IN);

    // Default scaling: 20% (good for MCUs)
    TCS3200_SetFrequencyScaling(TCS3200_SCALE_20_PERCENT);
    // Default filter: CLEAR (all photodiodes)
    TCS3200_SetFilter(TCS3200_FILTER_CLEAR);
}

void TCS3200_SetFrequencyScaling(tcs3200_scale_t scale)
{
    tcs3200_set_s0_s1(scale);
}

void TCS3200_SetFilter(tcs3200_filter_t filter)
{
    tcs3200_set_s2_s3(filter);
}

uint32_t TCS3200_ReadFrequencyHz(uint32_t gate_time_ms)
{
    if (gate_time_ms == 0)
        return 0;

    const uint32_t gate_time_us = gate_time_ms * 1000u;
    const uint32_t start_sync = time_us_32();
    uint32_t now = start_sync;

    // Sync to first rising edge on OUT to start counting
    bool last = gpio_get(TCS3200_OUT_PIN);
    uint32_t sync_timeout_us = 100000u; // 100 ms timeout
    while ((now - start_sync) < sync_timeout_us)
    {
        bool current = gpio_get(TCS3200_OUT_PIN);
        if (!last && current)
        {
            last = current;
            break;
        }
        last = current;
        now = time_us_32();
    }

    // Count rising edges
    uint32_t gate_start = time_us_32();
    uint32_t gate_end = gate_start + gate_time_us;
    uint32_t count = 0;

    while ((now = time_us_32()) < gate_end)
    {
        bool current = gpio_get(TCS3200_OUT_PIN);
        if (!last && current)
        {
            count++;
        }
        last = current;
    }

    // Calculate Frequency
    uint32_t freq_hz = (count * 1000u) / gate_time_ms;
    return freq_hz;
}

void TCS3200_ReadRGB(uint32_t gate_time_ms,
                     uint32_t *r_hz,
                     uint32_t *g_hz,
                     uint32_t *b_hz)
{
    if (!r_hz || !g_hz || !b_hz)
        return;

    // RED
    TCS3200_SetFilter(TCS3200_FILTER_RED);
    sleep_ms(10); // Allow settling
    *r_hz = TCS3200_ReadFrequencyHz(gate_time_ms);

    // GREEN
    TCS3200_SetFilter(TCS3200_FILTER_GREEN);
    sleep_ms(10);
    *g_hz = TCS3200_ReadFrequencyHz(gate_time_ms);

    // BLUE
    TCS3200_SetFilter(TCS3200_FILTER_BLUE);
    sleep_ms(10);
    *b_hz = TCS3200_ReadFrequencyHz(gate_time_ms);

    // Reset to CLEAR
    TCS3200_SetFilter(TCS3200_FILTER_CLEAR);
}