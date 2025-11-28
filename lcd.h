#ifndef LCD_H
#define LCD_H

#include "pico/stdlib.h"

// --- FIXED PIN MAPPING ---
#define LCD_PIN_RS 0
#define LCD_PIN_EN 1
#define LCD_PIN_D4 2
#define LCD_PIN_D5 3
#define LCD_PIN_D6 4
#define LCD_PIN_D7 5

void lcd_init(void);
void lcd_clear(void);
void lcd_set_cursor(int row, int col);
void lcd_string(const char *s);
void lcd_char(char c);

#endif