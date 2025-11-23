#include "button.h"

void button_init(void)
{
    gpio_init(BUTTON_PIN);
    gpio_set_dir(BUTTON_PIN, GPIO_IN);

    // Enable the internal pull-up resistor.
    // This means the pin reads 1 (High) when open, and 0 (Low) when pressed.
    gpio_pull_up(BUTTON_PIN);
}

bool button_is_pressed(void)
{
    // Since we used a pull-up, the button connects to Ground when pressed.
    // So, logic 0 means pressed.
    // We return true if gpio_get is false (0).
    return !gpio_get(BUTTON_PIN);
}