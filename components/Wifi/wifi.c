#include "wifi.h"
#include "CoreVariables.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_netif.h"

/* Tag for any esp logs*/
static const char *TAG = "WIFI";

static EventGroupHandle_t WifiEvtGroup;

/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

/* Local Function Declarations*/
void vWifi_Init();
static void wifi_event_handler(void *arg, esp_event_base_t event_source, int32_t event_type, void *event_data);

void Wifi_Core(void *pvParameters)
{
    /* Initialization the Wi-Fi*/
    vWifi_Init();

    /* Main body of the Wifi Task*/
    while(1)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void vWifi_Init()
{

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    if (CONFIG_LOG_MAXIMUM_LEVEL > CONFIG_LOG_DEFAULT_LEVEL) {
        /* If you only want to open more logs in the wifi module, you need to make the max level greater than the default level,
         * and call esp_log_level_set() before esp_wifi_init() to improve the log level of the wifi module. */
        esp_log_level_set("wifi", CONFIG_LOG_MAXIMUM_LEVEL);
    }

    ESP_LOGI(TAG, "ESP_WIFI_MODE_STA");
    WifiEvtGroup = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());

    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, &instance_got_ip));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASSWORD,
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_init_sta finished.");

    /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
     * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
    EventBits_t wifi_evt_bits = xEventGroupWaitBits(WifiEvtGroup,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdTRUE,
            pdFALSE,
            portMAX_DELAY);

    /* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
     * happened. */
    if (wifi_evt_bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "connected to ap SSID:%s password:%s",
                 WIFI_SSID, WIFI_PASSWORD);
    } else if (wifi_evt_bits & WIFI_FAIL_BIT) {
        ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s",
                 WIFI_SSID, WIFI_PASSWORD);
    } else {
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
    }   
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