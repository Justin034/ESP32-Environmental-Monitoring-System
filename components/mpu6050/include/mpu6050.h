#ifndef MPU6050_H
#define MPU6050_H

#include "driver/i2c_master.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

i2c_master_dev_handle_t init_mpu6050(i2c_master_bus_handle_t master);


#endif