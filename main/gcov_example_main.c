/* Blink Example with covergae info

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_app_trace.h"
#include "sdkconfig.h"

/* Can use project configuration menu (idf.py menuconfig) to choose the GPIO
   to blink, or you can edit the following line and set a number here.
*/
#define BLINK_GPIO 25

void blink_dummy_func(void);
void some_dummy_func(void);

static void blink_task(void *pvParameter)
{
    /*
    gpio_reset_pin(16);
    gpio_reset_pin(17);
    gpio_reset_pin(25);
    gpio_reset_pin(26);
    */
    gpio_set_direction(16, GPIO_MODE_OUTPUT);
    gpio_set_direction(17, GPIO_MODE_OUTPUT);
    gpio_set_direction(25, GPIO_MODE_OUTPUT);
    gpio_set_direction(26, GPIO_MODE_OUTPUT);
    gpio_set_level(16, 1);
    gpio_set_level(17, 1);
    gpio_set_level(25, 1);
    gpio_set_level(26, 1);
    while(1) {
        gpio_set_level(16, 0);
        vTaskDelay(2000 / portTICK_PERIOD_MS);
        gpio_set_level(16, 1);
        gpio_set_level(17, 0);
        vTaskDelay(2000 / portTICK_PERIOD_MS);
        gpio_set_level(17, 1);
        gpio_set_level(25, 0);
        vTaskDelay(2000 / portTICK_PERIOD_MS);
        gpio_set_level(25, 1);
        gpio_set_level(26, 0);
        vTaskDelay(2000 / portTICK_PERIOD_MS);
        gpio_set_level(26, 1);
        //gpio_set_level(25, 1);
        //vTaskDelay(2000 / portTICK_PERIOD_MS);
        //gpio_set_level(26, 0);
        //vTaskDelay(2000 / portTICK_PERIOD_MS);

        //gpio_set_level(26, 1);
        //vTaskDelay(2000 / portTICK_PERIOD_MS);
        /*
        blink_dummy_func();
        some_dummy_func();
        if (dump_gcov_after++ < 0) {
            // Dump gcov data
            printf("Ready to dump GCOV data...\n");
            esp_gcov_dump();
            printf("GCOV data have been dumped.\n");
        }
        */
    }
}

void app_main(void)
{
    xTaskCreate(&blink_task, "blink_task", configMINIMAL_STACK_SIZE, NULL, 5, NULL);
}
