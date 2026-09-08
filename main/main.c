#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "dht11.h"
#include "ds3231.h"
#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "lcd.h"
#include "mpu6050.h"
#include <esp_timer.h>
#include <esp_err.h>

#define BUTTON_GPIO GPIO_NUM_0       // Pushbutton GPIO
#define DEBOUNCE_DELAY_US 200000ULL  // Debounce delay in microseconds (200 ms)
#define DHT_PIN GPIO_NUM_14

static volatile uint64_t last_isr_time = 0;
static volatile uint32_t counter = 0;
char printBuf[16] = {0};

i2c_master_dev_handle_t MPU6050_handle;
i2c_master_dev_handle_t LCD_handle;
i2c_master_dev_handle_t DS3231_handle;
dht11_data_t data;
uint8_t databuf[6] = {0};
bool flag = false;

typedef enum
{
    MODE_A = 0,
    MODE_B,
    MODE_C

} system_mode_t;

static volatile system_mode_t current_mode = MODE_A;

static TaskHandle_t control_task_handle = NULL;

//----------------------------------------
// ACTIVITY A
//----------------------------------------

void activity_A(void)
{
    printf("Running Activity A\n");

    // Temp storage and resets
    uint8_t temp[7] = {0};
    uint8_t start = 0x00;

    // Extract time
    i2c_master_receive(DS3231_handle, temp, 7, -1);
    i2c_master_transmit(DS3231_handle, &start, 1, -1);

    // Display day of week
    char* day = dayFinder(temp[3]);
    unsigned int date = (unsigned int)(temp[4]);
    unsigned int month = (unsigned int)(temp[5]);
    sprintf(printBuf, "%s, %02X/%02X", day, date, month);
    lcd_set_cursor(0,0);
    lcd_write_string(printBuf);

    // Convert times and display
    unsigned int seconds = (unsigned int)(temp[0]);
    unsigned int minutes = (unsigned int)(temp[1]);
    unsigned int hour = (unsigned int)(temp[2]&0b00111111);
    sprintf(printBuf, "Time: %02X:%02X:%02X", hour, minutes, seconds);
    lcd_set_cursor(0,1);
    lcd_write_string(printBuf);

}

//----------------------------------------
// ACTIVITY B
//----------------------------------------

void activity_B(void)
{
    printf("Running Activity B\n");
    // DHT Check
    int check = dht11_read(&data);

    // Print or error check
    if(check == 0) {
        sprintf(printBuf, "Temp:%d Hum:%d", data.temperature, data.humidity);
        lcd_set_cursor(0, 1);
        lcd_write_string(printBuf);
    } else {
        printf("Error. Possibly due to fast refresh.");
    }
    vTaskDelay(pdMS_TO_TICKS(500));

}

//----------------------------------------
// ACTIVITY C
//----------------------------------------

void activity_C(void)
{
    printf("Running Activity C\n");

    // Clearing lines
    lcd_clear_line(1);
    
    uint8_t write = 0x43;
    ESP_ERROR_CHECK(i2c_master_transmit_receive(MPU6050_handle, &write, 1, databuf, 6, -1));

    int16_t arr[3] = {0};
    dht11_convert(&databuf[0], arr);
    
    sprintf(printBuf, "x:%d", arr[0]);
    lcd_set_cursor(0,1);
    lcd_write_string(printBuf);
    vTaskDelay(pdMS_TO_TICKS(500));
    
}

static void IRAM_ATTR button_isr(void *arg)
{
    BaseType_t higher_priority_task_woken = pdFALSE;

    // Notify control task
    vTaskNotifyGiveFromISR(
        control_task_handle,
        &higher_priority_task_woken
    );

    // Context switch if needed
    if (higher_priority_task_woken)
    {
        portYIELD_FROM_ISR();
    }
}

void control_task(void *pvParameters)
{
    while (1)
    {
        // Wait for button interrupt notification
        ulTaskNotifyTake(
            pdTRUE,
            portMAX_DELAY
        );

        // Toggle system mode
        if (current_mode == MODE_A)
        {
            current_mode = MODE_B;
            printf("\n=== SWITCHED TO MODE_B ===\n\n");
        }
        else if (current_mode == MODE_B)
        {
            current_mode = MODE_C;
            printf("\n=== SWITCHED TO MODE_C ===\n\n");
        }
        else
        {
            current_mode = MODE_A;
            printf("\n=== SWITCHED TO MODE_A ===\n\n");
        }
    }
}

//----------------------------------------
// WORKER TASK
//----------------------------------------

void worker_task(void *pvParameters)
{
    while (1)
    {
        switch (current_mode)
        {
            case MODE_A:
                activity_A();
                break;

            case MODE_B:
                activity_B();
                break;

            case MODE_C:
                activity_C();
                break;

            default:
                break;

        }
    }
}

//----------------------------------------
// MAIN
//----------------------------------------

void app_main(void)
{
    // Configure Button GPIO
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_POSEDGE, // Rising edge interrupt trigger
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = (1ULL << BUTTON_GPIO),
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_ENABLE
    };
    gpio_config(&io_conf);

    // Install GPIO ISR service
    gpio_install_isr_service(0);

    // Add ISR handler for button
    gpio_isr_handler_add(BUTTON_GPIO, button_isr, NULL);


    // Init. dht params
    
    dht11_init(DHT_PIN);

    // Bus Config
    i2c_master_bus_config_t i2c_mst_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = 0,
        .scl_io_num = 22,
        .sda_io_num = 21,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };

    i2c_master_bus_handle_t bus_handle;
    ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_mst_config, &bus_handle));

    // MPU init
    MPU6050_handle = init_mpu6050(bus_handle);

    // LCD init
    LCD_handle = init_lcd(bus_handle);
    lcdBootUp(bus_handle, LCD_handle);
    lcd_backlight(true);

    // DS3231 init
    DS3231_handle = init_ds3231(bus_handle);


    // DS3231 Setup
    uint8_t arr[8];
    uint8_t base = 0;
    arr[0] = 0x00; // Setup
    arr[1] = 0x45; // Seconds
    arr[2] = 0x01; // Minutes
    arr[3] = 0b00100001; // Hours (24hr)
    arr[4] = 0x01; // Day of week
    arr[5] = 0b00010110; // Date
    arr[6] = 0b00000101; // Month
    arr[7] = 0b00100110; // Year

    ESP_ERROR_CHECK(i2c_master_transmit(DS3231_handle, arr, sizeof(arr), -1));
    i2c_master_transmit(DS3231_handle, &base, 1, -1);

    // uint8_t time[4] = {0};

    // get_time(time);
    // set_time(time, DS3231_handle);
    
    printf("Presets are done. Commencing main loop.\n");

    xTaskCreate(
        control_task,
        "control_task",
        2048,
        NULL,
        10,
        &control_task_handle
    );

    xTaskCreate(
        worker_task,
        "worker_task",
        4096,
        NULL,
        5,
        NULL
    );
}