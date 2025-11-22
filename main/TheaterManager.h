#ifndef THEATER_MANAGER_H
#define THEATER_MANAGER_H

/* Includes for the Theater Manager */
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"

#include "CoreDefines.h"

/* An enum of all of the possible high level events (tv control, led control, etc. */
typedef enum 
{
    THEATER_EVT_NONE = 0,
    THEATER_EVT_SET_SCENE,  /* Set a default scene for the theater */
    THEATER_EVT_TV_CONTROL, /* A control wants to be sent to the TV (volunme, etc if possible )*/
    THEATER_EVT_X           /* Other events we will want */
} theater_event_type_t;

/* Enum for the different "default scenes". These would be *
 * pre-set selectable options for the vibe of the theater  */
typedef enum 
{
    THEATER_SCENE_IDLE = 0,
    THEATER_SCENE_MOVIE,   /* Scene should be currently selected movie configuration set by the user*/
    THEATER_SCENE_COZY,   /* Maybe a default "cozy" can be configured by the user*/
    THEATER_SCENE_INTERMISSION /* Movie intermission, snack/bathroom break */
} theater_scenes_t;

/* Main event struct? ASK CHATGPT MORE */
typedef struct 
{
    theater_event_type_t evt_type;
        
    union 
    {
        struct 
        {
            theater_scenes_t scene;
        } scene;
        
        struct 
        {
            uint8_t level;
        } level;
    } data;
} theater_event_t;

#endif // THEATER_MANAGER_H