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
#include "esp_tls.h"
#include "esp_crt_bundle.h"


#define TELEGRAM_TOKEN CONFIG_TELEGRAM_TOKEN
#define TELEGRAM_CHAT_ID_ACCESS_LIST CONFIG_TELEGRAM_CHAT_ID_ACCESS_LIST
#define TELEGRAM_HTTP_API_URL CONFIG_TELEGRAM_HTTP_API_URL


struct relay_struct {
    int gpio_output_number;
    int gpio_output_level;
    int gpio_input_number;
    int gpio_input_last;
    char relay_name[80];
    char *relay_emoji;
};
struct relay_struct relay_list[4];

int relay_count = 4;
int offset = 0;
bool reboot_status = false;

static const char *TAG = "example";

int port_detect(char *port_number) {
    
    if ((!strcmp("0", port_number)) ||
        (!strcmp("1", port_number)) ||
        (!strcmp("2", port_number)) ||
        (!strcmp("3", port_number))) {
        return( port_number[0] - '0');
    } else {
        return(42);
    }
    
}

char* urlencode(char* originalText)
{
    // allocate memory for the worst possible case (all characters need to be encoded)
    char *encodedText = (char *)malloc(sizeof(char)*strlen(originalText)*3+1);
    
    const char *hex = "0123456789abcdef";
    
    int pos = 0;
    for (int i = 0; i < strlen(originalText); i++) {
        if (('a' <= originalText[i] && originalText[i] <= 'z')
            || ('A' <= originalText[i] && originalText[i] <= 'Z')
            || ('0' <= originalText[i] && originalText[i] <= '9')) {
                encodedText[pos++] = originalText[i];
            } else {
                encodedText[pos++] = '%';
                encodedText[pos++] = hex[originalText[i] >> 4];
                encodedText[pos++] = hex[originalText[i] & 15];
            }
    }
    encodedText[pos] = '\0';
    return encodedText;
}

static esp_err_t api_relay_handler(httpd_req_t *req)
{
    char*  buf;
    size_t buf_len;
    char   api_metod[12];
    char   api_relay_port[8];
    int output_level = 2;
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
                    output_level = 1;
                } else if (!strcmp("off", api_metod)) {
                    output_level = 0;
                }
                
                if (output_level != 2) {
                    int port_number = port_detect(api_relay_port);
                    if (port_number != 42) {
                        printf("port: %d\n", port_number);
                        relay_list[port_number].gpio_output_level = output_level;
                        gpio_set_level(relay_list[port_number].gpio_output_number, output_level);
                    } else if (!strcmp("all", api_relay_port)) {
                        for (int i = 0; i<relay_count; i++)
                        {
                            relay_list[i].gpio_output_level = output_level;
                            gpio_set_level(relay_list[i].gpio_output_number, output_level);
                        }
                    }
                }
            }
            if (!strcmp("status", api_metod)) {
                output_level = 3;
            }
        }
        free(buf);
    }

    if (output_level != 2 ) {
        char port_str[8];
        char status_str[8];
        char name_str[8];
        
        for (int i = 0; i<relay_count; i++)
        {
            sprintf(port_str, "%d", i);
            sprintf(name_str, "relay_%d", i);
            if (relay_list[i].gpio_output_level == 1) {
                sprintf(status_str, "%s", "on");
            } else if (relay_list[i].gpio_output_level == 0) {
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
    printf("free_heap_size_stop: %d\n", esp_get_free_heap_size());
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

void radio_task(void *pvParameter)
{
    relay_list[0].gpio_output_number = 5;
    relay_list[0].gpio_input_number  = 2;
    relay_list[1].gpio_output_number = 4;
    relay_list[1].gpio_input_number  = 3;
    relay_list[2].gpio_output_number = 8;
    relay_list[2].gpio_input_number  = 10;
    relay_list[3].gpio_output_number = 9;
    relay_list[3].gpio_input_number  = 6;
    relay_list[0].relay_emoji = "0⃣";
    relay_list[1].relay_emoji = "1⃣";
    relay_list[2].relay_emoji = "2⃣";
    relay_list[3].relay_emoji = "3⃣";
    int i;
    int input_level;
    for (i = 0; i<relay_count; i++)
    {
        gpio_reset_pin(relay_list[i].gpio_output_number);
        gpio_reset_pin(relay_list[i].gpio_input_number);
        gpio_set_direction(relay_list[i].gpio_output_number, GPIO_MODE_OUTPUT);
        gpio_set_direction(relay_list[i].gpio_input_number, GPIO_MODE_INPUT);
        relay_list[i].gpio_output_level = gpio_get_level(relay_list[i].gpio_output_number);
        relay_list[i].gpio_input_last = gpio_get_level(relay_list[i].gpio_input_number);
    }
    
    nvs_handle_t my_handle;
    esp_err_t err = nvs_open("storage", NVS_READWRITE, &my_handle);
    if (err != ESP_OK) {
        printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
    } else {
        size_t required_size;
        char relay_key[18];
        
        for (i = 0; i<relay_count; i++)
        {
            sprintf(relay_key, "relay_%d", i);
            nvs_get_str(my_handle, relay_key, NULL,&required_size);
            nvs_get_str(my_handle, relay_key, (char *)&relay_list[i].relay_name, &required_size);
        }
        nvs_close(my_handle);
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
                printf("free_heap_size: %d\n", esp_get_free_heap_size());
            }
        }
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}

static void get_updates()
{
    long long int Timer1 = esp_timer_get_time();
    cJSON* cjson_content = NULL;
    cJSON* cjson_content_result = NULL;
    cJSON* cjson_content_update_id = NULL;
    cJSON* cjson_content_message = NULL;
    cJSON* cjson_content_message_chat = NULL;
    cJSON* cjson_content_message_chat_id = NULL;
    cJSON* cjson_content_message_text = NULL;
    int update_id = 0;
    bool send_message = false;
    int chat_id;
    char *url;
    url = malloc(800);
    sprintf(url, "%s/bot%s/getUpdates?limit=1&offset=%d",
        TELEGRAM_HTTP_API_URL,
        TELEGRAM_TOKEN,
        offset
    );
    printf("url: %s\n", url);
    
    char output_buffer[512] = {0};
    int content_length = 0;
    esp_http_client_config_t config = {
        .url = url,
        .crt_bundle_attach = esp_crt_bundle_attach,
        .buffer_size = 1024,
        .buffer_size_tx = 1024,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);

    // GET Request
    esp_http_client_set_method(client, HTTP_METHOD_GET);
    free(url);
    esp_http_client_set_header(client, "Host", "api.telegram.org");
    esp_err_t err = esp_http_client_open(client, 0);
    
    if (err != ESP_OK) {
        printf("Failed to open HTTP connection: %s\n", esp_err_to_name(err));
    } else {
        content_length = esp_http_client_fetch_headers(client);
        if (content_length > 23) {
            
            int data_read = esp_http_client_read_response(client, output_buffer, 1024);
            
            if (data_read >= 0) {
                printf("HTTP GET Status = %d, content_length = %d\n",
                esp_http_client_get_status_code(client),
                esp_http_client_get_content_length(client));
                //printf("Response: %s\n", output_buffer);
                cjson_content = cJSON_Parse(output_buffer);
            } else {
                printf("Failed to read response\n");
            }
        }
    }
    esp_http_client_close(client);
    esp_http_client_cleanup(client);
    if (reboot_status == true) {
        printf("Reboot system!\n");
        abort();
    }
    if(cjson_content != NULL)
    {
        cjson_content_result = cJSON_GetArrayItem(cJSON_GetObjectItem(cjson_content, "result"), 0);
        cjson_content_update_id = cJSON_GetObjectItemCaseSensitive(cjson_content_result, "update_id");
        cjson_content_message = cJSON_GetObjectItem(cjson_content_result, "message");
        cjson_content_message_chat = cJSON_GetObjectItem(cjson_content_message, "chat");
        cjson_content_message_chat_id = cJSON_GetObjectItemCaseSensitive(cjson_content_message_chat, "id");
        cjson_content_message_text = cJSON_GetObjectItemCaseSensitive(cjson_content_message, "text");
        
        if (cJSON_IsString(cjson_content_message_text)) {
            char *message = cjson_content_message_text->valuestring;
            char *message_strtok = strtok(message, " ");
            int arguments_count = 3;
            char *arguments[arguments_count];
            int output_level = 2;
            int port_number;
            
            int i = 0;
            while ((message_strtok != NULL) && (i<arguments_count)) {
                printf ("message: %s\n", message);
                arguments[i] = message_strtok;
                message_strtok = strtok(NULL, " ");
                i++;
            }
            
            if (!strcmp("reboot", arguments[0])) {
                reboot_status = true;
            } else if (!strcmp("on", arguments[0]) || !strcmp("On", arguments[0])) {
                output_level = 1;
            } else if (!strcmp("off", arguments[0]) || !strcmp("Off", arguments[0])) {
                output_level = 0;
            } else if (!strcmp("set_name", arguments[0])) {
                port_number = port_detect(arguments[1]);
                if (port_number != 42 && arguments[2][0] != '\0') {
                    nvs_handle_t my_handle;
                    esp_err_t err = nvs_open("storage", NVS_READWRITE, &my_handle);
                    if (err != ESP_OK) {
                        printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
                    } else {
                        printf ("set_name_%d: %s\n", port_number, arguments[2]);
                        sprintf(relay_list[port_number].relay_name, "%s", arguments[2]);
                        char relay_key[12];
                        sprintf(relay_key, "relay_%d", port_number);
                        nvs_set_str(my_handle, relay_key, relay_list[port_number].relay_name);
                        nvs_close(my_handle);
                    }
                }
            }
            if (output_level != 2) {
                port_number = port_detect(arguments[1]);
                if (port_number != 42 ) {
                    printf("port: %d\n", port_number);
                    relay_list[port_number].gpio_output_level = output_level;
                    gpio_set_level(relay_list[port_number].gpio_output_number, output_level);
                } else if (!strcmp("all", arguments[1])) {
                    for (int i = 0; i<relay_count; i++)
                    {
                        relay_list[i].gpio_output_level = output_level;
                        gpio_set_level(relay_list[i].gpio_output_number, output_level);
                    }
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
        if (cJSON_IsNumber(cjson_content_update_id)) {
            update_id = cjson_content_update_id->valueint;
            printf("update_id: %d\n", update_id);
        }
        if (send_message) {
            printf("free_heap_size_start: %d\n", esp_get_free_heap_size());
            char *url_message;
            url_message = malloc(3072);
            char *relay;
            char status_str[32];
            
            sprintf(url_message, "%s/bot%s/sendMessage?chat_id=%d&text=",
                TELEGRAM_HTTP_API_URL,
                TELEGRAM_TOKEN,
                cjson_content_message_chat_id->valueint
            );
            
            if (reboot_status == true) {
                strcat(url_message, urlencode("Reboot system!"));
            } else {
                for (int i = 0; i<relay_count; i++)
                {
                    if (relay_list[i].gpio_output_level == 1) {
                        sprintf(status_str, "%s", "🌕"); //on
                    } else if (relay_list[i].gpio_output_level == 0) {
                        sprintf(status_str, "%s", "🌑"); //off
                    }
                    relay = malloc(200);
                    sprintf(relay, "%s%s %s\n", relay_list[i].relay_emoji, status_str, relay_list[i].relay_name);
                    strcat(url_message, urlencode(relay));
                    free(relay);
                }
            }
            
            printf("strlen url_message: %d\n", strlen(url_message));
            
            esp_http_client_config_t config = {
                .url = url_message,
                .crt_bundle_attach = esp_crt_bundle_attach,
                .buffer_size = 4096,
                .buffer_size_tx = 4096,
            };
            esp_http_client_handle_t client = esp_http_client_init(&config);
            free(url_message);
            esp_http_client_set_header(client, "Host", "api.telegram.org");
            esp_err_t err;
            
            do {
                err = esp_http_client_perform(client);
            }while(err == ESP_ERR_HTTP_EAGAIN);
            
            if (err == ESP_OK) {
                offset = update_id+1;
                printf("HTTP Status = %d, content_length = %d\n",
                        esp_http_client_get_status_code(client),
                        esp_http_client_get_content_length(client));
            } else {
                printf("Error perform http request %s", esp_err_to_name(err));
            }
            
            esp_http_client_cleanup(client);
            printf("free_heap_size_stop: %d\n", esp_get_free_heap_size());
        }
    }
    cJSON_Delete(cjson_content);
    printf("Difference: %lld ms\n", (esp_timer_get_time()-Timer1)/1000);
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
    
    while(true){
        printf("free_heap_size: %d\n", esp_get_free_heap_size());
        
        for (int i = 0; i<10; i++)
        {
            printf("step %d\n", i);
            get_updates();
        }
    }
}
