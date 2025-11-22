#include "TheaterManager.h"

/* Tag shown for esp log messages for debugging*/
#ifdef DEBUG_CFG
    static const char *TAG = "THEATER MANAGER";
#endif

/* The global event queue for the theater manager*/
static QueueHandle_t g_EvtQueue = NULL;

/* Helper function for anyone to post events to the queue */
static bool post_to_theater_bus(const theater_event_t *evt, uint16_t timeout_ms)
{
    /* Check to ensure that the queue exists. If it does not, return false */
    if(!g_EvtQueue)
    {
        return false;
    }
    
    /* Return true if the command was successfully queued. If any error, return false */
    return xQueueSend(g_EvtQueue, evt, timeout_ms) == pdTRUE;
}

typedef enum
{
    LIGHTING_MODE_NORMAL = 0,
    LIGHTING_MODE_CINEMA,
    LIGHTING_MODE_AMBIENT
} lighting_mode_t;