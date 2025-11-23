#include "lcd.h"

// Helper: Pulse the Enable pin to tell LCD to read data
void lcd_toggle_enable(void)
{
    gpio_put(LCD_PIN_EN, 1);
    sleep_us(50); // Increased delay for stability (Standard is ~0.5us, we use 50us to be safe)
    gpio_put(LCD_PIN_EN, 0);
    sleep_us(50); // Wait for data to latch
}

// Helper: Send 4 bits of data to D4-D7
void lcd_send_nibble(uint8_t nibble)
{
    gpio_put(LCD_PIN_D4, (nibble >> 0) & 0x01);
    gpio_put(LCD_PIN_D5, (nibble >> 1) & 0x01);
    gpio_put(LCD_PIN_D6, (nibble >> 2) & 0x01);
    gpio_put(LCD_PIN_D7, (nibble >> 3) & 0x01);
    lcd_toggle_enable();
}

// Helper: Send a full byte (split into two 4-bit nibbles)
// mode: 0 = Command, 1 = Data (Character)
void lcd_send_byte(uint8_t val, int mode)
{
    gpio_put(LCD_PIN_RS, mode);

    // Send High Nibble (Most Significant 4 bits)
    lcd_send_nibble(val >> 4);

    // Send Low Nibble (Least Significant 4 bits)
    lcd_send_nibble(val & 0x0F);

    // Execution delay. Most commands need >37us.
    // We use 200us to be absolutely safe against gibberish.
    sleep_us(200);
}

void lcd_clear(void)
{
    lcd_send_byte(0x01, 0);
    sleep_ms(2); // Clear command requires a full 2ms delay
}

void lcd_set_cursor(int row, int col)
{
    // 0x80 is the command to set cursor address
    // Row 0 starts at 0x00, Row 1 starts at 0x40
    uint8_t val = (row == 0) ? (0x80 + col) : (0xC0 + col);
    lcd_send_byte(val, 0);
}

void lcd_char(char c)
{
    lcd_send_byte(c, 1); // 1 = Data mode
}

void lcd_string(const char *s)
{
    while (*s)
    {
        lcd_char(*s++);
    }
}

void lcd_init(void)
{
    // 1. Setup GPIO
    gpio_init(LCD_PIN_RS);
    gpio_set_dir(LCD_PIN_RS, GPIO_OUT);
    gpio_init(LCD_PIN_EN);
    gpio_set_dir(LCD_PIN_EN, GPIO_OUT);
    gpio_init(LCD_PIN_D4);
    gpio_set_dir(LCD_PIN_D4, GPIO_OUT);
    gpio_init(LCD_PIN_D5);
    gpio_set_dir(LCD_PIN_D5, GPIO_OUT);
    gpio_init(LCD_PIN_D6);
    gpio_set_dir(LCD_PIN_D6, GPIO_OUT);
    gpio_init(LCD_PIN_D7);
    gpio_set_dir(LCD_PIN_D7, GPIO_OUT);

    gpio_put(LCD_PIN_RS, 0);
    gpio_put(LCD_PIN_EN, 0);

    // 2. Power-on delay (LCD needs time to boot)
    sleep_ms(100);

    // 3. Initialization Sequence (The "Magic" Reset)
    // We must send 0x03 three times to ensure the LCD enters 8-bit mode
    // properly before switching to 4-bit mode.

    // Sequence 1
    lcd_send_nibble(0x03);
    sleep_ms(5);

    // Sequence 2
    lcd_send_nibble(0x03);
    sleep_us(150);

    // Sequence 3
    lcd_send_nibble(0x03);
    sleep_us(150);

    // 4. Switch to 4-bit mode
    lcd_send_nibble(0x02);
    sleep_us(150);

    // 5. Configure Display
    // Function Set: 4-bit mode, 2 lines, 5x8 font
    lcd_send_byte(0x28, 0);

    // Display Control: Display OFF, Cursor OFF, Blink OFF
    lcd_send_byte(0x08, 0);

    // Clear Display
    lcd_clear();

    // Entry Mode Set: Increment cursor, no shift
    lcd_send_byte(0x06, 0);

    // Display Control: Display ON, Cursor OFF, Blink OFF
    lcd_send_byte(0x0C, 0);
}