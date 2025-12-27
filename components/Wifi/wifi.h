#ifndef WIFI_H
#define WIFI_H

#include "stdbool.h"

/* Defines */
#define WIFI_SSID      "TnT Dungeon"
#define WIFI_PASSWORD  "TayloafTbot1431"

/* Function Declarations*/
extern void Wifi_Core(void *pvParameters);

/* Enumeration that holds the state of the Wi-Fi of the esp32 */
typedef enum
{
    WIFI_STATE_IDLE = 0,
    WIFI_STATE_CONNECTING,
    WIFI_STATE_CONNECTED,
    WIFI_STATE_DISCONNECTED
} wifi_state_t;

/* Wi-Fi struct that holds all of the main Wi-Fi information */
typedef struct
{
    wifi_state_t state;
    bool         has_ip;
    esp_netif_ip_info_t ip_info;
    wifi_err_reason_t reason_for_disconnect;
} wifi_status_t;

#endif // WIFI_H