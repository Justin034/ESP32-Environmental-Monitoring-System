#include <stdio.h>
#include "ds3231.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

char* dayFinder(uint8_t dOfW) {
    if(dOfW == 1) {
        return "Saturday";
    } else if(dOfW == 2) {
        return "Sunday";
    } else if(dOfW == 3) {
        return "Monday";
    } else if(dOfW == 4) {
        return "Tuesday";
    } else if(dOfW == 5) {
        return "Wednesday";
    } else if(dOfW == 6) {
        return "Thursday";
    } else if(dOfW == 7) {
        return "Friday";
    }
    return NULL;
}

i2c_master_dev_handle_t init_ds3231(i2c_master_bus_handle_t master)
{
    i2c_device_config_t i2c_device_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = 0x68,
        .scl_speed_hz = 400000,
    };

    i2c_master_dev_handle_t i2c_bus_handle;
    ESP_ERROR_CHECK(i2c_master_bus_add_device(master, &i2c_device_config, &i2c_bus_handle));

    vTaskDelay(pdMS_TO_TICKS(50)); // LCD power-up delay

    return i2c_bus_handle;
}

void get_time(uint8_t* arr) {

    arr[0] = 0x00;
    time_t t;
    time(&t);
    struct tm* tracker = localtime(&t);
    printf("Current local time and date: %s", asctime(tracker));

    uint8_t seconds_tens = (uint8_t)tracker->tm_sec / 10;
    uint8_t seconds  = (uint8_t)tracker->tm_sec % 10;

    // seconds
    arr[1] = seconds_tens << 4 | seconds;

    uint8_t min_tens = (uint8_t)tracker->tm_min / 10;
    uint8_t min  = (uint8_t)tracker->tm_min % 10;

    // minutes
    arr[2] = min_tens << 4 | min;

    uint8_t hr_tens = (uint8_t)tracker->tm_hour / 10;
    uint8_t hr  = (uint8_t)tracker->tm_hour % 10;

    // hours
    arr[3] = hr_tens << 4 | hr;
}

void set_time(uint8_t* arr, i2c_master_dev_handle_t handle) {
    uint8_t base = 0x00;

    ESP_ERROR_CHECK(i2c_master_transmit(handle, arr, sizeof(arr), -1));
    i2c_master_transmit(handle, &base, 1, -1);

}