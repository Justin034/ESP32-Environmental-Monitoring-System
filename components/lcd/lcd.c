#include "lcd.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/i2c_master.h"

static const char *TAG = "I2C_LCD";

static i2c_master_dev_handle_t i2c_device_handle = NULL;
static i2c_master_bus_handle_t i2c_bus_handle = NULL;
static uint8_t lcd_backlight_status = LCD_BACKLIGHT;

// Low-level enable toggle
static esp_err_t i2c_transmit_with_enable_toggle(uint8_t data)
{
    uint8_t data_with_enable = data | LCD_ENABLE;
    ESP_ERROR_CHECK(i2c_master_transmit(i2c_device_handle, &data_with_enable, 1, -1));
    vTaskDelay(pdMS_TO_TICKS(1));

    data_with_enable &= ~LCD_ENABLE;
    ESP_ERROR_CHECK(i2c_master_transmit(i2c_device_handle, &data_with_enable, 1, -1));
    vTaskDelay(pdMS_TO_TICKS(1));

    return ESP_OK;
}

// Send 4-bit chunks
static esp_err_t i2c_send_byte_on_4bits(uint8_t data, uint8_t rs)
{
    uint8_t nibbles[2] = {
        (data & 0xF0) | rs | lcd_backlight_status | LCD_RW_WRITE,
        ((data << 4) & 0xF0) | rs | lcd_backlight_status | LCD_RW_WRITE
    };

    ESP_ERROR_CHECK(i2c_transmit_with_enable_toggle(nibbles[0]));
    ESP_ERROR_CHECK(i2c_transmit_with_enable_toggle(nibbles[1]));

    vTaskDelay(pdMS_TO_TICKS(1));
    return ESP_OK;
}

// I2C Master Init
i2c_master_dev_handle_t init_lcd(i2c_master_bus_handle_t master)
{
    i2c_device_config_t i2c_device_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = LCD_I2C_ADDRESS,
        .scl_speed_hz = I2C_MASTER_FREQ_HZ
    };

    i2c_master_dev_handle_t lcd_bus_handle;
    ESP_ERROR_CHECK(i2c_master_bus_add_device(master, &i2c_device_config, &lcd_bus_handle));

    vTaskDelay(pdMS_TO_TICKS(50)); // LCD power-up delay
    ESP_LOGI(TAG, "I2C LCD initialized");

    i2c_bus_handle = master;
    i2c_device_handle = lcd_bus_handle;

    return lcd_bus_handle;
}

// LCD Init
void lcdBootUp(i2c_master_bus_handle_t data1, i2c_master_dev_handle_t data2)
{
    i2c_device_handle = data2;
    i2c_bus_handle = data1;
    vTaskDelay(pdMS_TO_TICKS(50)); // power-up delay

    lcd_backlight_status = LCD_BACKLIGHT;

    // Function set for 1602 (2 lines!)
    uint8_t init_commands[] = {
        0x28, // 4-bit, 2-line, 5x8 font
        0x0C, // display ON, cursor OFF
        0x01, // clear display
        0x06  // entry mode: increment cursor
    };

    for (int i = 0; i < sizeof(init_commands); i++) {
        ESP_ERROR_CHECK(i2c_send_byte_on_4bits(init_commands[i], LCD_RS_CMD));
        vTaskDelay(pdMS_TO_TICKS(5));
    }

    vTaskDelay(pdMS_TO_TICKS(5));
}

// Set cursor location
void lcd_set_cursor(uint8_t col, uint8_t row)
{
    if (row > 1) row = 1;     // 1602 only 2 rows
    if (col > 15) col = 15;   // 16 columns

    uint8_t row_offsets[] = {0x00, 0x40};

    uint8_t cmd = 0x80 | (col + row_offsets[row]);

    ESP_ERROR_CHECK(i2c_send_byte_on_4bits(cmd, LCD_RS_CMD));
}

// Write string
void lcd_write_string(const char *str)
{
    while (*str) {
        ESP_ERROR_CHECK(i2c_send_byte_on_4bits((uint8_t)*str, LCD_RS_DATA));
        str++;
    }
}

// Clear display (Leads to flickering so maybe ditch)
void lcd_clear(void)
{
    ESP_ERROR_CHECK(i2c_send_byte_on_4bits(0x01, LCD_RS_CMD));
}

// Backlight control
void lcd_backlight(bool state)
{
    if (state) {
        lcd_backlight_status |= LCD_BACKLIGHT;
    } else {
        lcd_backlight_status &= ~LCD_BACKLIGHT;
    }

    ESP_ERROR_CHECK(i2c_master_transmit(i2c_device_handle, &lcd_backlight_status, 1, -1));
}

// Clearing just lines
void lcd_clear_line(uint8_t row) {
    lcd_set_cursor(0, row);

    for (int i = 0; i < 16; i++) {
        ESP_ERROR_CHECK(
            i2c_send_byte_on_4bits(' ', LCD_RS_DATA)
        );
    }

    lcd_set_cursor(0, row);
}