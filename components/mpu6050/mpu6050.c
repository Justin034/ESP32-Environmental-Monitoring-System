#include <stdio.h>
#include "mpu6050.h"

i2c_master_dev_handle_t init_mpu6050(i2c_master_bus_handle_t master) {

    // device config.
    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = 0x69,
        .scl_speed_hz = 400000,
    };

    i2c_master_dev_handle_t MPU6050_handle;
    ESP_ERROR_CHECK(i2c_master_bus_add_device(master, &dev_cfg, &MPU6050_handle));

    vTaskDelay(pdMS_TO_TICKS(500));

    return MPU6050_handle;
}