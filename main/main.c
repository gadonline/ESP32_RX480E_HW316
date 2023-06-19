/* Simple HTTP + SSL Server Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <esp_wifi.h>
#include <esp_event.h>
#include <esp_log.h>
#include <esp_system.h>
#include <nvs_flash.h>
#include <sys/param.h>
#include "esp_netif.h"
#include "esp_eth.h"
#include "protocol_examples_common.h"

#include <esp_http_server.h>
#include "esp_tls.h"


#include "esp_bt.h"
#include "esp_gap_ble_api.h"
#include "esp_gattc_api.h"
#include "esp_gatt_defs.h"
#include "esp_bt_main.h"
#include "esp_gatt_common_api.h"

#include "cJSON.h"


#include <string.h>
#include <stdlib.h>
#include "math.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "protocol_examples_common.h"
#include "protocol_examples_utils.h"
#include "esp_tls.h"
#include "esp_crt_bundle.h"

#include "esp_http_client.h"
#include "driver/gpio.h"

#define GATTC_TAG "GATTC_DEMO"
#define REMOTE_SERVICE_UUID        0x00FF
#define REMOTE_NOTIFY_CHAR_UUID    0xFF01
#define PROFILE_NUM      1
#define PROFILE_A_APP_ID 0
#define INVALID_HANDLE   0
#define TELEGRAM_TOKEN CONFIG_TELEGRAM_TOKEN
#define TELEGRAM_CHAT_ID_ACCESS_LIST CONFIG_TELEGRAM_CHAT_ID_ACCESS_LIST
#define TELEGRAM_HTTP_PROXY_SERVER CONFIG_TELEGRAM_HTTP_PROXY_SERVER
#define EXAMPLE_HTTP_QUERY_KEY_MAX_LEN  (64)


struct relay_struct {
    int gpio_output_number;
    int gpio_output_level;
    int gpio_input_number;
    int gpio_input_last;
};
struct relay_struct relay_list[4];

int relay_count = 4;

static const char *TAG = "example";

static esp_err_t api_relay_handler(httpd_req_t *req)
{
    char*  buf;
    size_t buf_len;
    char   api_metod[8];
    char   api_relay_port[8];
    int    relay_port[4];
    buf_len = httpd_req_get_url_query_len(req) + 1;
    if (buf_len > 1) {
        buf = malloc(buf_len);
        if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
            ESP_LOGI(TAG, "Found URL query => %s", buf);
            if ((httpd_query_key_value(buf, "metod", api_metod, sizeof(api_metod)) == ESP_OK) &&
                (httpd_query_key_value(buf, "port", api_relay_port, sizeof(api_relay_port)) == ESP_OK)) {
                if ((!strcmp("1", api_relay_port)) ||
                    (!strcmp("2", api_relay_port)) ||
                    (!strcmp("3", api_relay_port)) ||
                    (!strcmp("4", api_relay_port))) {
                    relay_port[0] = api_relay_port[0] - '0';
                    printf("relay_port: %d\n", relay_port[0]);
                } else if (!strcmp("all", api_relay_port)) {
                    relay_port[0] = 1;
                    relay_port[1] = 2;
                    relay_port[2] = 3;
                    relay_port[3] = 4;
                    printf("relay_ports: %d,%d,%d,%d\n", relay_port[0], relay_port[1], relay_port[2], relay_port[3]);
                }
                int i;
                if (relay_port[0]) {
                    if (!strcmp("on", api_metod)) {
                        printf("HTTP API metod: on\n");
                        for(i = 0; i < sizeof(relay_port) / sizeof(relay_port[0]); i++)
                        {
                            if (relay_port[i] == 0) {
                                break;
                            }
                            printf("API port: %d\n", relay_port[i]);
                            gpio_set_level(relay_list[relay_port[i]].gpio_output_number, 0);
                        }
                    } else if (!strcmp("off", api_metod)) {
                        printf("HTTP API metod: off\n");
                        for(i = 0; i < sizeof(relay_port) / sizeof(relay_port[0]); i++)
                        {
                            if (relay_port[i] == 0) {
                                break;
                            }
                            printf("API port: %d\n", relay_port[i]);
                            gpio_set_level(relay_list[relay_port[i]].gpio_output_number, 1);
                        }
                    } else if (!strcmp("status", api_metod)) {
                        printf("HTTP API metod: status\n");
                    }
                }
            }
        }
        free(buf);
    }

    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, "ESP32_LYWSD03MMC\n", HTTPD_RESP_USE_STRLEN);
    
    return ESP_OK;
}

static const httpd_uri_t api_relay = {
    .uri       = "/api/relay",
    .method    = HTTP_GET,
    .handler   = api_relay_handler
};


static httpd_handle_t start_webserver(void)
{
    httpd_handle_t server = NULL;

    ESP_LOGI(TAG, "Starting server");

    httpd_config_t conf = HTTPD_DEFAULT_CONFIG();

    esp_err_t ret = httpd_start(&server, &conf);
    if (ESP_OK != ret) {
        ESP_LOGI(TAG, "Error starting server!");
        return NULL;
    }

    ESP_LOGI(TAG, "Registering URI handlers");
    httpd_register_uri_handler(server, &api_relay);
    return server;
}

static void stop_webserver(httpd_handle_t server)
{
    httpd_stop(server);
}

static void disconnect_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    httpd_handle_t* server = (httpd_handle_t*) arg;
    if (*server) {
        stop_webserver(*server);
        *server = NULL;
    }
}

static void connect_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    httpd_handle_t* server = (httpd_handle_t*) arg;
    if (*server == NULL) {
        *server = start_webserver();
    }
}

void app_main(void)
{
    static httpd_handle_t server = NULL;

    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    
#ifdef CONFIG_EXAMPLE_CONNECT_WIFI
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &connect_handler, &server));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &disconnect_handler, &server));
#endif // CONFIG_EXAMPLE_CONNECT_WIFI
#ifdef CONFIG_EXAMPLE_CONNECT_ETHERNET
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_ETH_GOT_IP, &connect_handler, &server));
    ESP_ERROR_CHECK(esp_event_handler_register(ETH_EVENT, ETHERNET_EVENT_DISCONNECTED, &disconnect_handler, &server));
#endif // CONFIG_EXAMPLE_CONNECT_ETHERNET
    ESP_ERROR_CHECK(example_connect());
    
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
                gpio_set_level(relay_list[i].gpio_output_number, relay_list[i].gpio_output_level);
                relay_list[i].gpio_input_last = input_level;
                printf("relay %d set level: %d\n", i, relay_list[i].gpio_output_level);
                printf("free_heap_size: %lu\n", esp_get_free_heap_size());
            }
        }
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}
