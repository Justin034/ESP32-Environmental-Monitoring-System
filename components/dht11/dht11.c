#include "dht11.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_rom_sys.h"

static gpio_num_t dht_pin;

void dht11_init(gpio_num_t pin)
{
    dht_pin = pin;
    gpio_reset_pin(dht_pin);
    // Active-low: HIGH = idle ("off"), LOW = start ("on")
    gpio_set_direction(dht_pin, GPIO_MODE_OUTPUT);
    gpio_set_level(dht_pin, 1); // idle HIGH ("off")
}

int clip(int v, int min, int max)
{
    if (v < min) return min;
    if (v > max) return max;
    return v;
}

void dht11_convert(uint8_t* databuf, int16_t* array) {
    int16_t x = (int16_t)((databuf[0]<<8) | databuf[1]);
    int16_t y = (int16_t)((databuf[2]<<8) | databuf[3]);
    int16_t z = (int16_t)((databuf[4]<<8) | databuf[5]);
    x /= 131;
    y /= 131;
    z /= 131;
    x = clip(x, -100, 100);
    y = clip(y, -100, 100);
    z = clip(z, -100, 100);

    array[0] = x;
    array[1] = y;
    array[2] = z;
}

int dht11_read(dht11_data_t *data)
{
    uint8_t bits[5] = {0};
    int i, j;

    // ------------------
    // Start signal
    // ------------------
    gpio_set_direction(dht_pin, GPIO_MODE_OUTPUT);
    gpio_set_level(dht_pin, 0);       // "on"
    vTaskDelay(20 / portTICK_PERIOD_MS); // 18–20 ms
    gpio_set_level(dht_pin, 1);       // "off"
    esp_rom_delay_us(40);             // 20–40 µs
    gpio_set_direction(dht_pin, GPIO_MODE_INPUT);

    // ------------------
    // Wait for DHT11 response
    // ------------------
    uint32_t timeout = 1000;

    // DHT pulls LOW for 80 µs
    while (gpio_get_level(dht_pin) == 1 && timeout--) esp_rom_delay_us(1);
    if (timeout == 0) return -2;

    timeout = 1000;
    // DHT pulls HIGH for 80 µs
    while (gpio_get_level(dht_pin) == 0 && timeout--) esp_rom_delay_us(1);
    if (timeout == 0) return -2;

    timeout = 1000;
    while (gpio_get_level(dht_pin) == 1 && timeout--) esp_rom_delay_us(1);
    if (timeout == 0) return -2;

    // ------------------
    // Read 40 bits
    // ------------------
    for (i = 0; i < 5; i++) {
        for (j = 0; j < 8; j++) {
            // Wait for LOW pulse (~50 µs)
            timeout = 1000;
            while (gpio_get_level(dht_pin) == 0 && timeout--) esp_rom_delay_us(1);
            if (timeout == 0) return -2;

            // Measure HIGH pulse length
            uint32_t t = 0;
            while (gpio_get_level(dht_pin) == 1 && t < 1000) {
                esp_rom_delay_us(1);
                t++;
            }

            // >40 µs → bit 1, else bit 0
            if (t > 40) bits[i] |= (1 << (7 - j));
        }
    }

    // ------------------
    // Validate checksum
    // ------------------
    if (bits[4] != (uint8_t)(bits[0] + bits[1] + bits[2] + bits[3]))
        return -1;

    data->humidity = bits[0];
    data->temperature = bits[2];

    return 0;
}