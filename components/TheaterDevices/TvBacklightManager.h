#ifndef TV_BACKLIGHT_MANAGER_H
#define TV_BACKLIGHT_MANAGER_H

#include <stdint.h>
#include "led_strip.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

/* Power Enable Pin for the Backlight LED strip */
#define BACKLIGHT_STRIP_ENABLE_PIN       (6)
#define GPIO_INPUT_PIN_SELECT            (1ULL << BACKLIGHT_STRIP_ENABLE_PIN)

/* The GPIO Pin that the LED strip's data line is connected to */
#define TV_BACKLIGHT_PIN                 (7)

/* Queue used for the tv backlight LEDs */
extern QueueHandle_t TvBacklight_Queue;

/* The possible commands for the TV Backlight LED strip*/
typedef enum 
{
    /* Power Commands */
    TV_BACKLIGHT_CMD_POWER_DOWN,
    TV_BACKLIGHT_CMD_POWER_ON,

    /* Brightness Command */
    TV_BACKLIGHT_CMD_SET_BRIGHTNESS,

    /* Strip LED commands */
    TV_BACKLIGHT_CMD_SOLID_STRIP,
    TV_BACKLIGHT_CMD_SINGLE_LED,
    TV_BACKLIGHT_CMD_REFRESH
} tv_backlight_action_t;

/* This struct stores the value of each color per LED */
typedef struct
{
    /* The LED on the strip will read, 8 bits for each color in this order */
    uint8_t red;
    uint8_t green;
    uint8_t blue;
    uint8_t white;
} backlight_led_values_t;

typedef struct
{
    tv_backlight_action_t action;

    union
    {
        uint8_t BrightnessPct;

        struct 
        {
            backlight_led_values_t color;
        } fill_strip;

        struct 
        {
            uint16_t index;
            backlight_led_values_t color;
        } one_led;
    } Data;

} tv_backlight_cmd_with_info_t;

/* This will be the main baclkight struct for one LED strip with all relevant info. Things need to be moved in here */
typedef struct
{
    /* Is the strip on or off? */
    bool bPoweredOn;
    uint8_t u8BrightnessPct;

    /* The strip object that is managed by the led_strip component */
    led_strip_handle_t Object;
} BacklightStrip_t;

extern BacklightStrip_t MainTvBacklightStrip;

/* Function Declarations */
esp_err_t TvBacklight_Init(void);
esp_err_t TvBacklight_SetLED_Color(uint16_t index, backlight_led_values_t color);
esp_err_t Set_TvBacklight_Strip_To_Constant_Color(backlight_led_values_t color);
esp_err_t Refresh_TvBacklight_Strip(void);
esp_err_t Clear_TvBacklight_Strip(void);
void      PowerBacklightStripOff(void);
void      TvBacklight_Task(void *pv);

#endif // TV_BACKLIGHT_MANAGER_H
