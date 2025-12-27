#ifndef TV_BACKLIGHT_MANAGER_H
#define TV_BACKLIGHT_MANAGER_H

#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

/* The number of LEDs on the TV backlight strip */
#define NUM_BACKLIGHT_LEDS 300

/* FIXME: The GPIO Pin that the LED strip's data line is connected to */
#define TV_BACKLIGHT_PIN 7

/* FreeRTOS task and queue used for the tv backlight LEDs */
static QueueHandle_t q_TvBacklight_Queue;
static TaskHandle_t  t_TvBacklight_TaskHandle;

/* The possible commands for the TV Backlight LED strip*/
typedef enum 
{
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
        struct 
        {
            backlight_led_values_t color;
        } fill_strip;

        struct 
        {
            uint16_t index;
            backlight_led_values_t color;
        } one_led;
    } set_led;

} tv_backlight_cmd_with_info_t;

/* Function Declarations */
esp_err_t TvBacklight_Init(void);
esp_err_t TvBacklight_SetLED_Color(uint16_t index, backlight_led_values_t color);
esp_err_t Set_TvBacklight_Strip_To_Constant_Color(backlight_led_values_t color);
esp_err_t Refresh_TvBacklight_Strip(void);
esp_err_t Clear_TvBacklight_Strip(void);

#endif // TV_BACKLIGHT_MANAGER_H