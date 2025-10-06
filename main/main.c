#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_err.h"
#include "wifi.h"

void app_main(void)
{
    // Will want to initialize some stuff up here i'm sure before starting tasks

    xTaskCreate(Wifi_Core, "Wifi Core Task", 1024 * 4, NULL, 0, NULL); 
}