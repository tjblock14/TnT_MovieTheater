#include "network_events.h"

#include <esp_log.h>

static const char *TAG = "NETWORK_EVT";

EventGroupHandle_t g_NetworkEventGroup = NULL;

void network_events_init(void)
{
    if(g_NetworkEventGroup == NULL)
    {
        g_NetworkEventGroup = xEventGroupCreate();

        if(g_NetworkEventGroup == NULL)
        {
            ESP_LOGE(TAG, "Failed to create network event group");
        }
    }
}