#ifndef LCD
#define LCD

#include "pico/stdlib.h"

// -----------------------------------------------------------------------
// HARDWARE CONFIGURATION
// CHANGE THESE PINS TO MATCH YOUR EXACT WIRING
// -----------------------------------------------------------------------
#define LCD_PIN_RS 2
#define LCD_PIN_EN 3
#define LCD_PIN_D4 4
#define LCD_PIN_D5 5
#define LCD_PIN_D6 6
#define LCD_PIN_D7 7

// -----------------------------------------------------------------------
// FUNCTIONS
// -----------------------------------------------------------------------

// Initialize the LCD (Call this first!)
void lcd_init(void);

// Clear the screen
void lcd_clear(void);

// Set cursor position (0 = top row, 1 = bottom row)
void lcd_set_cursor(int row, int col);

// Write a string
void lcd_string(const char *s);

// Write a single character
void lcd_char(char c);

#endif