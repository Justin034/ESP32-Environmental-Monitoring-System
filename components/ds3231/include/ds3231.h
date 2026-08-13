#ifndef DS3231_H
#define DS3231_H

#include <time.h>
#include "driver/i2c_master.h"

char* dayFinder(uint8_t dOfW);

i2c_master_dev_handle_t init_ds3231(i2c_master_bus_handle_t master);

void get_time(uint8_t* arr);

void set_time(uint8_t* arr, i2c_master_dev_handle_t handle);




#endif