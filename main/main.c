#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_err.h"
#include "wifi.h"

#include "TvBacklightManager.h"

backlight_led_values_t color =
        {
            .blue = 255,
            .green = 0,
            .red = 0,
            .white = 0
        };

void app_main(void)
{
    // Will want to initialize some stuff up here i'm sure before starting tasks

    //xTaskCreate(Wifi_Core, "Wifi Core Task", 1024 * 4, NULL, 0, NULL); 

    TvBacklight_Init();
    vTaskDelay(pdMS_TO_TICKS(1000));
    Clear_TvBacklight_Strip();
    vTaskDelay(pdMS_TO_TICKS(1000));
    /* Temporary while 1 loop for testing */
    while(1)
    {
        
        TvBacklight_SetLED_Color(0, color);
        Refresh_TvBacklight_Strip();

        color.blue -= 1;
        color.red += 1;

        vTaskDelay(pdMS_TO_TICKS(50));
    } 
}