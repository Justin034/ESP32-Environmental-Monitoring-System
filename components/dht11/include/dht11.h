#pragma once
#include "driver/gpio.h"

typedef struct {
    int temperature;   // in °C
    int humidity;      // in %
} dht11_data_t;

int clip(int v, int min, int max);

// Initialize DHT11 on a given GPIO
void dht11_init(gpio_num_t pin);

// Read DHT11 data
// Returns 0 on success, -1 on checksum error, -2 on timeout
int dht11_read(dht11_data_t *data);

void dht11_convert(uint8_t* databuf, int16_t* array);