#ifndef BUTTON_H
#define BUTTON_H

#include "pico/stdlib.h"

// Define the GPIO pin for the button
// You can change this to whatever pin you are using
#define BUTTON_PIN 15

// Initialize the button pin (sets as Input and enables Pull-Up)
void button_init(void);

// Returns true if the button is currently pressed
bool button_is_pressed(void);

#endif