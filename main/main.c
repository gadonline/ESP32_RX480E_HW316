/* Simple HTTP + SSL Server Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <esp_wifi.h>
#include <esp_log.h>
#include <nvs_flash.h>
#include <sys/param.h>
#include "protocol_examples_common.h"
#include <esp_http_server.h>
#include "esp_http_client.h"
#include "cJSON.h"
#include "driver/gpio.h"

#define TELEGRAM_TOKEN CONFIG_TELEGRAM_TOKEN
#define TELEGRAM_CHAT_ID_ACCESS_LIST CONFIG_TELEGRAM_CHAT_ID_ACCESS_LIST
#define TELEGRAM_HTTP_PROXY_SERVER CONFIG_TELEGRAM_HTTP_PROXY_SERVER


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
    char   api_metod[12];
    char   api_relay_port[8];
    int level = 2;
    buf_len = httpd_req_get_url_query_len(req) + 1;
    cJSON* relay_list_json = cJSON_CreateObject();
    cJSON* relay_json = NULL;
    if (buf_len > 1) {
        buf = malloc(buf_len);
        if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
            ESP_LOGI(TAG, "Found URL query => %s", buf);
            if ((httpd_query_key_value(buf, "metod", api_metod, sizeof(api_metod)) == ESP_OK) &&
                (httpd_query_key_value(buf, "port", api_relay_port, sizeof(api_relay_port)) == ESP_OK)) {
                if (!strcmp("on", api_metod)) {
                    level = 0;
                } else if (!strcmp("off", api_metod)) {
                    level = 1;
                }
                
                if ((level == 0 || level == 1) &&
                   ((!strcmp("0", api_relay_port)) ||
                    (!strcmp("1", api_relay_port)) ||
                    (!strcmp("2", api_relay_port)) ||
                    (!strcmp("3", api_relay_port)) ||
                    (!strcmp("4", api_relay_port)))) {
                    int port_i = api_relay_port[0] - '0';
                    printf("relay_port: %d\n", port_i);
                    if (port_i == 4) {
                        for (int i = 0; i<relay_count; i++)
                        {
                            relay_list[i].gpio_output_level = level;
                            gpio_set_level(relay_list[i].gpio_output_number, relay_list[i].gpio_output_level);
                        }
                    } else {
                        relay_list[port_i].gpio_output_level = level;
                        gpio_set_level(relay_list[port_i].gpio_output_number, relay_list[port_i].gpio_output_level);
                    }
                }
            }
            if (!strcmp("status", api_metod)) {
                level = 3;
            }
        }
        free(buf);
    }

    if (level != 2 ) {
        char port_str[8];
        char status_str[8];
        char name_str[8];
        
        for (int i = 0; i<relay_count; i++)
        {
            sprintf(port_str, "%d", i);
            sprintf(name_str, "relay_%d", i);
            if (relay_list[i].gpio_output_level == 0) {
                sprintf(status_str, "%s", "on");
            } else if (relay_list[i].gpio_output_level == 1) {
                sprintf(status_str, "%s", "off");
            }
            relay_json = cJSON_CreateObject();
            cJSON_AddStringToObject(relay_json, "port", port_str);
            cJSON_AddStringToObject(relay_json, "status", status_str);
            cJSON_AddItemToObject(relay_list_json, name_str, relay_json);
        }
    }
    char *relay_list_json_print = cJSON_PrintUnformatted(relay_list_json);
    cJSON_Delete(relay_list_json);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, relay_list_json_print, HTTPD_RESP_USE_STRLEN);
    free(relay_list_json_print);
    return ESP_OK;
}

static esp_err_t telegram_post_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, "ESP32_RX480E_HW316\n", HTTPD_RESP_USE_STRLEN);
    
    char *content;
    content = malloc(500);
    size_t recv_size = MIN(req->content_len, 500);

    int ret = httpd_req_recv(req, content, recv_size);
    printf("recv_size: %d\n", recv_size);
    printf("ret: %d\n", ret);
    if (ret != 0) {
        cJSON* cjson_content = NULL;
        cJSON* cjson_content_message = NULL;
        cJSON* cjson_content_message_chat = NULL;
        cJSON* cjson_content_message_chat_id = NULL;
        cJSON* cjson_content_message_text = NULL;
        
        cjson_content = cJSON_Parse(content);
        free(content);
        
        if(cjson_content != NULL)
        {
            cjson_content_message = cJSON_GetObjectItem(cjson_content, "message");
            cjson_content_message_chat = cJSON_GetObjectItem(cjson_content_message, "chat");
            cjson_content_message_chat_id = cJSON_GetObjectItemCaseSensitive(cjson_content_message_chat, "id");
            cjson_content_message_text = cJSON_GetObjectItemCaseSensitive(cjson_content_message, "text");
            bool send_message = false;
            int chat_id;
            
            if (cJSON_IsString(cjson_content_message_text)) {
                char *message = cjson_content_message_text->valuestring;
                printf("message: %s\n", message);
                if (!strcmp("reboot", message)) {
                    abort();
                } else if (!strcmp("on 0", message)) {
                    relay_list[0].gpio_output_level = 0;
                    gpio_set_level(relay_list[0].gpio_output_number, relay_list[0].gpio_output_level);
                } else if (!strcmp("on 1", message)) {
                    relay_list[1].gpio_output_level = 0;
                    gpio_set_level(relay_list[1].gpio_output_number, relay_list[1].gpio_output_level);
                } else if (!strcmp("on 2", message)) {
                    relay_list[2].gpio_output_level = 0;
                    gpio_set_level(relay_list[2].gpio_output_number, relay_list[2].gpio_output_level);
                } else if (!strcmp("on 3", message)) {
                    relay_list[3].gpio_output_level = 0;
                    gpio_set_level(relay_list[3].gpio_output_number, relay_list[3].gpio_output_level);
                } else if (!strcmp("on 4", message)) {
                    for (int i = 0; i<relay_count; i++)
                    {
                        relay_list[i].gpio_output_level = 0;
                        gpio_set_level(relay_list[i].gpio_output_number, relay_list[i].gpio_output_level);
                    }
                } else if (!strcmp("off 0", message)) {
                    relay_list[0].gpio_output_level = 1;
                    gpio_set_level(relay_list[0].gpio_output_number, relay_list[0].gpio_output_level);
                } else if (!strcmp("off 1", message)) {
                    relay_list[1].gpio_output_level = 1;
                    gpio_set_level(relay_list[1].gpio_output_number, relay_list[1].gpio_output_level);
                } else if (!strcmp("off 2", message)) {
                    relay_list[2].gpio_output_level = 1;
                    gpio_set_level(relay_list[2].gpio_output_number, relay_list[2].gpio_output_level);
                } else if (!strcmp("off 3", message)) {
                    relay_list[3].gpio_output_level = 1;
                    gpio_set_level(relay_list[3].gpio_output_number, relay_list[3].gpio_output_level);
                } else if (!strcmp("off 4", message)) {
                    for (int i = 0; i<relay_count; i++)
                    {
                        relay_list[i].gpio_output_level = 1;
                        gpio_set_level(relay_list[i].gpio_output_number, relay_list[i].gpio_output_level);
                    }
                }
            }
            
            if (cJSON_IsNumber(cjson_content_message_chat_id)) {
                chat_id = cjson_content_message_chat_id->valueint;
                char str[] = TELEGRAM_CHAT_ID_ACCESS_LIST;
                char *istr = strtok(str, ",");
                
                while (istr != NULL)
                {
                    if (atoi(istr) == chat_id) {
                        send_message = true;
                        break;
                    }
                    istr = strtok(NULL, ",");
                }
            }
            
            if (send_message) {
                printf("free_heap_size_start: %lu\n", esp_get_free_heap_size());
                char *url;
                url = malloc(800);
                char *relay;
                char status_str[8];
                
                sprintf(url, "http://%s/bot%s/sendMessage?chat_id=%d&text=", TELEGRAM_HTTP_PROXY_SERVER, TELEGRAM_TOKEN, cjson_content_message_chat_id->valueint);
                
                for (int i = 0; i<relay_count; i++)
                {
                    //: %%3A
                    //\n %%0A
                    if (relay_list[i].gpio_output_level == 0) {
                        sprintf(status_str, "%s", "on");
                    } else if (relay_list[i].gpio_output_level == 1) {
                        sprintf(status_str, "%s", "off");
                    }
                    relay = malloc(100);
                    sprintf(relay, "relay_%d%%3A%%20%s%%0A", i, status_str);
                    strcat(url, relay);
                    free(relay);
                }
                
                printf("url: %s\n", url);
                
                esp_http_client_config_t config = {
                    .url = url,
                    .buffer_size = 1024,
                    .buffer_size_tx = 1024,
                };
                esp_http_client_handle_t client = esp_http_client_init(&config);
                free(url);
                esp_http_client_set_header(client, "Host", "api.telegram.org");
                esp_err_t err;
                
                do {
                    err = esp_http_client_perform(client);
                }while(err == ESP_ERR_HTTP_EAGAIN);
                
                if (err == ESP_OK) {
                    printf("HTTPS Status = %d, content_length = %lld\n",
                            esp_http_client_get_status_code(client),
                            esp_http_client_get_content_length(client));
                } else {
                    printf("Error perform http request %s", esp_err_to_name(err));
                }
                
                esp_http_client_cleanup(client);
                printf("free_heap_size_stop: %lu\n", esp_get_free_heap_size());
            }
        }
        cJSON_Delete(cjson_content);
    } else {
        free(content);
    }
    
    return ESP_OK;
}

static const httpd_uri_t api_relay = {
    .uri       = "/api/relay",
    .method    = HTTP_GET,
    .handler   = api_relay_handler
};

static const httpd_uri_t telegram = {
    .uri       = "/ESP32_RX480E_HW316",
    .method    = HTTP_POST,
    .handler   = telegram_post_handler
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
    httpd_register_uri_handler(server, &telegram);
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

void radio_task(void *pvParameter)
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
        relay_list[i].gpio_output_level = gpio_get_level(relay_list[i].gpio_output_number);
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

void app_main(void)
{
    static httpd_handle_t server = NULL;

    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    
    xTaskCreate(radio_task, "blink_task", 2048, NULL, 5, NULL);
    
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &connect_handler, &server));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &disconnect_handler, &server));
    ESP_ERROR_CHECK(example_connect());
}
