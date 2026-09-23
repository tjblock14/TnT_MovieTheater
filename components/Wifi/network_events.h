#ifndef NETWORK_EVENTS_H
#define NETWORK_EVENTS_H

#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>

/* Function declarations */
void network_events_init(void);

/* Network event group handle */
extern EventGroupHandle_t g_NetworkEventGroup;

/* Bit locations of the event group */
#define NET_EVT_WIFI_STARTED      (1 << 0)
#define NET_EVT_WIFI_CONNECTED    (1 << 1)
#define NET_EVT_GOT_IP            (1 << 2)

#endif // NETWORK_EVENTS_H