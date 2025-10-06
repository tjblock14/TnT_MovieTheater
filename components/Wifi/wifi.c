#include "wifi.h"
#include "CoreVariables.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_netif.h"

/* Tag for any esp logs*/
static const char *TAG = "WIFI";

/* Local Function Declarations*/
void vWifi_Init();
static void wifi_event_handler(void *arg, esp_event_base_t event_source, int32_t event_type, void *event_data);

void Wifi_Core(void *pvParameters)
{
    /* Initializations before entering the body of the task*/
    vWifi_Init();

    /* Main body of the Wifi Task*/
    while(1)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void vWifi_Init()
{
    esp_err_t err;

    err = nvs_flash_init();
    if(err != ESP_OK)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }
    
    ESP_ERROR_CHECK(esp_netif_init()); // Initialize the TCP/IP adapter (network stack)
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    
    /* Create the default wi-fi station network interface. Station (sta) means it *
     * will act as a device (phone, laptop, etc) that connects to the router      */
    esp_netif_t *sta = esp_netif_create_default_wifi_sta();
    assert(sta);

    /* Configure the default wi-fi info struct (driver I think) to default settings */
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

    /* Set the wi-fi credentials. The wi-fi driver will use this to connect to the router*/
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASSWORD,
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config)); // Load the wi-fi name and password

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL));
    
    /* Start the wi-fi driver*/
    ESP_ERROR_CHECK(esp_wifi_start());

    /* Log that we have initialized the wi-fi*/
    ESP_LOGI(TAG, "wifi_init_sta finished.");
}

static void wifi_event_handler(void *arg, esp_event_base_t event_source, int32_t event_type, void *event_data)
{
    if((event_source == WIFI_EVENT) && (event_type == WIFI_EVENT_STA_START))
    {
        ESP_LOGI(TAG, "Station start -> connecting...");
        esp_wifi_connect();
    }
    else if((event_source == WIFI_EVENT) && (event_type == WIFI_EVENT_STA_CONNECTED))
    {
        ESP_LOGI(TAG, "Connected to AP");
    }
    else if((event_source == WIFI_EVENT) && (event_type == WIFI_EVENT_STA_DISCONNECTED))
    {
        wifi_event_sta_disconnected_t *sta_disconnect =  (wifi_event_sta_disconnected_t *)event_data;
        ESP_LOGW(TAG, "Disconnected, reason = %d. Reconnecting...", sta_disconnect->reason);
        esp_wifi_connect();
    }
    else if((event_source == IP_EVENT) && (event_type == IP_EVENT_STA_GOT_IP))
    {
        ip_event_got_ip_t *ip_data = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&ip_data->ip_info.ip));
    }
}