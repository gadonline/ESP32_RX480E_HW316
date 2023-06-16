#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_app_trace.h"
#include "sdkconfig.h"

struct relay_struct {
    int gpio_output_number;
    int gpio_output_level;
    int gpio_input_number;
    int gpio_input_last;
};
struct relay_struct relay_list[4];

int relay_count = 4;

void app_main(void)
{
    relay_list[0].gpio_output_number = 16;
    relay_list[0].gpio_input_number  = 34;
    relay_list[1].gpio_output_number = 17;
    relay_list[1].gpio_input_number  = 35;
    relay_list[2].gpio_output_number = 25;
    relay_list[2].gpio_input_number  = 36;
    relay_list[3].gpio_output_number = 26;
    relay_list[3].gpio_input_number  = 39;
    
    int i;
    int input_level;
    for (i = 0; i<relay_count; i++)
    {
        gpio_set_direction(relay_list[i].gpio_output_number, GPIO_MODE_OUTPUT);
        gpio_set_direction(relay_list[i].gpio_input_number, GPIO_MODE_INPUT);
        gpio_set_level(relay_list[i].gpio_output_number, 1);
        relay_list[i].gpio_output_level = 1;
        relay_list[i].gpio_input_last = gpio_get_level(relay_list[i].gpio_input_number);
    }
    
    while(1) {
        for (i = 0; i<relay_count; i++)
        {
            input_level = gpio_get_level(relay_list[i].gpio_input_number);
            if (relay_list[i].gpio_input_last != input_level) {
                if (relay_list[i].gpio_output_level == 0) {
                    relay_list[i].gpio_output_level = 1;
                } else {
                    relay_list[i].gpio_output_level = 0;
                }
                printf("relay %d set level: %d\n", i, relay_list[i].gpio_output_level);
                gpio_set_level(relay_list[i].gpio_output_number, relay_list[i].gpio_output_level);
                relay_list[i].gpio_input_last = input_level;
            }
        }
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}
