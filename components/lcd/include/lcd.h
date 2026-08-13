#ifndef LCD_H
#define LCD_H

#include <stdint.h>
#include <stdbool.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c_master.h"
#include "driver/gpio.h"
#include "esp_log.h"

// =======================
// I2C Configuration
// =======================
#define I2C_MASTER_NUM        I2C_NUM_0
#define I2C_MASTER_SDA_IO     21
#define I2C_MASTER_SCL_IO     22
#define I2C_MASTER_FREQ_HZ    400000

#define LCD_I2C_ADDRESS       0x27

// =======================
// PCF8574 Bit Mapping
// (typical LCD backpack)
// =======================
//
// P0 = RS
// P1 = RW
// P2 = EN
// P3 = BACKLIGHT
// P4–P7 = D4–D7
//
// =======================

// Control bits
#define LCD_RS_CMD        (0 << 0)
#define LCD_RS_DATA       (1 << 0)

#define LCD_RW_WRITE      (0 << 1)
#define LCD_RW_READ       (1 << 1)

#define LCD_ENABLE        (1 << 2)
#define LCD_BACKLIGHT     (1 << 3)

// =======================
// Function Prototypes
// =======================
i2c_master_dev_handle_t init_lcd(i2c_master_bus_handle_t master);

void lcdBootUp(i2c_master_bus_handle_t data1, i2c_master_dev_handle_t data2);

void lcd_set_cursor(uint8_t col, uint8_t row);

void lcd_write_string(const char *str);

void lcd_clear(void);

void lcd_backlight(bool state);

void lcd_clear_line(uint8_t row);

#endif // LCD_H