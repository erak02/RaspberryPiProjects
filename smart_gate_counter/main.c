#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include <string.h>
#include <stdio.h>
#include "hardware/timer.h"

#define I2C_PORT i2c0

const uint SDA_PIN = 0;
const uint SCL_PIN = 1;
const uint PIR_PIN = 3;
const uint BUTTON_RESET = 4;
const uint GREEN_LED = 5;
const uint RED_LED = 6;

#define LCD_ADDR 0x27

/*LCD commands*/ 
#define LCD_FUNCTIONSET 0x20
#define LCD_DISPLAYCONTROL 0x08
#define LCD_CLEARDISPLAY 0x01
#define LCD_ENTRYMODESET 0x04
#define LCD_SETDDRAMADDR 0x80

/*function flags*/
#define LCD_2LINE 0x08

/*flags*/
#define LCD_DISPLAYON 0x04
#define LCD_ENTRYLEFT 0x02

#define LCD_BACKLIGHT 0x08
#define LCD_ENABLE 0x04
#define LCD_RS 0x01

volatile int car_counter = 0;
volatile bool screen_needs_update = false;
char count_string[32];
static alarm_id_t led_alarm = -1;

void i2c_write_byte(uint8_t data)
{
    i2c_write_blocking(I2C_PORT, LCD_ADDR, &data, 1, false);

}

void lcd_pulse_enable(uint8_t data)
{
    i2c_write_byte(data | LCD_ENABLE);
    sleep_us(500);

    i2c_write_byte(data & ~LCD_ENABLE);
    sleep_us(500);
}

void lcd_send_nibble(uint8_t nibble, uint8_t rs)
{
    uint8_t data = 0;
    data = data | (nibble & 0xF0);
    data = data | LCD_BACKLIGHT;

    if (rs)
    {
        data = data | LCD_RS;
    }

    i2c_write_byte(data);
    lcd_pulse_enable(data);
}

void lcd_write_byte(uint8_t val, uint8_t rs)
{
    lcd_send_nibble(val & 0xF0, rs);
    lcd_send_nibble((val<<4) & 0xF0, rs);
}

void lcd_init()
{
    sleep_ms(50);

    lcd_send_nibble(0x30, 0);
    sleep_us(150);

    lcd_send_nibble(0x30, 0);
    sleep_us(150);

    lcd_send_nibble(0x30, 0);
    sleep_us(150);

    lcd_send_nibble(0x20, 0);
    sleep_us(150);

    lcd_write_byte(LCD_FUNCTIONSET | LCD_2LINE, 0);
    lcd_write_byte(LCD_DISPLAYCONTROL | LCD_DISPLAYON, 0);
    lcd_write_byte(LCD_CLEARDISPLAY, 0);
    sleep_ms(2);
    lcd_write_byte(LCD_ENTRYMODESET | LCD_ENTRYLEFT, 0);
}

void lcd_set_cursor(int line, int position)
{
    if (position > 15)
    {
        position = 15;
    }

    int value;

    if(line == 0)
    {
        value = LCD_SETDDRAMADDR | position;
    }
    else if (line == 1)
    {
        value = LCD_SETDDRAMADDR | (position + 0x40);
    }
    lcd_write_byte(value, 0);
}

void lcd_char(char val)
{
    lcd_write_byte(val, 1);
}

void lcd_string(char s[])
{
    for(int i = 0; s[i] != '\0'; i++)
    {
        lcd_char(s[i]);
    }
}

void lcd_clear()
{
    lcd_write_byte(LCD_CLEARDISPLAY, 0);
    sleep_ms(2);
}

int64_t alarm_callback(alarm_id_t id, void *user_data)
{
    gpio_put(RED_LED, 0);
    gpio_put(GREEN_LED, 1);
    led_alarm = -1;
    return 0;
}

void gpio_irq_handler(uint gpio, uint32_t events)
{
    if (gpio == PIR_PIN && (events & GPIO_IRQ_EDGE_RISE))
    {
        car_counter++;
        screen_needs_update = true;

        gpio_put(GREEN_LED, 0);
        gpio_put(RED_LED, 1);

        if(led_alarm >= 0)
        {
            cancel_alarm(led_alarm);
        }

        led_alarm = add_alarm_in_ms(5000, alarm_callback, NULL, false);
    }
    else if (gpio == BUTTON_RESET && (events & GPIO_IRQ_EDGE_FALL))
    {
        car_counter = 0;
        screen_needs_update = true;
    }
}

int main()
{
    stdio_init_all();

    i2c_init(I2C_PORT, 100*1000);
    gpio_set_function(SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(SDA_PIN);
    gpio_pull_up(SCL_PIN);

    gpio_init(PIR_PIN);
    gpio_set_dir(PIR_PIN, GPIO_IN);
    gpio_pull_down(PIR_PIN);

    gpio_init(BUTTON_RESET);
    gpio_set_dir(BUTTON_RESET, GPIO_IN);
    gpio_pull_up(BUTTON_RESET);

    gpio_init(GREEN_LED);
    gpio_set_dir(GREEN_LED, GPIO_OUT);

    gpio_init(RED_LED);
    gpio_set_dir(RED_LED, GPIO_OUT);

    lcd_init();
    lcd_set_cursor(0,0);
    lcd_string("Number of cars");
    lcd_set_cursor(1, 0);
    lcd_string("detected:");

    gpio_set_irq_enabled_with_callback(PIR_PIN, GPIO_IRQ_EDGE_RISE, true, &gpio_irq_handler);
    gpio_set_irq_enabled_with_callback(BUTTON_RESET, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);

    gpio_put(GREEN_LED, 1);
    
    
    while(true)
    {
        tight_loop_contents();
        if(screen_needs_update)
        {
            lcd_set_cursor(1, 10);
            snprintf(count_string, sizeof(count_string), "%d", car_counter);
            lcd_string(count_string);
            screen_needs_update = false;
        }
    }
}