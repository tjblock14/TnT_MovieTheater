#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_err.h"

#include "wifi.h"
#include "udp_raw_receiver.h"
#include "network_events.h"

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
    /* Initialize the network event group so we can successfully *
     * begin the Wi-Fi and the UDP listener tasks                */
    network_events_init();
    xTaskCreate(Wifi_Core, "Wifi Core Task", 1024 * 4, NULL, 0, NULL); 
    xTaskCreate(udp_listen_task, "UDP Listen Task", 1024 * 6, NULL, 0, NULL);

    
     TvBacklight_Init();
    vTaskDelay(pdMS_TO_TICKS(1000));
    Clear_TvBacklight_Strip();
    vTaskDelay(pdMS_TO_TICKS(1000));
    /* Temporary while 1 loop for testing 
    while(1)
    {
        
        TvBacklight_SetLED_Color(0, color);
        Refresh_TvBacklight_Strip();

        color.blue -= 1;
        color.red += 1;

        vTaskDelay(pdMS_TO_TICKS(50));
    } 
    */
}