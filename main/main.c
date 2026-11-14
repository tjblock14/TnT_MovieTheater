#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_err.h"
#include "driver/gpio.h"

#include "BluetoothCore.h"
#include "wifi.h"
#include "udp_raw_receiver.h"
#include "network_events.h"

#include "TvBacklightManager.h"
#include "BluetoothCore.h"

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
    xTaskCreate(Wifi_Core, "Wifi Core Task", 1024 * 4, NULL, 3, NULL);
    xTaskCreate(udp_listen_task, "UDP Listen Task", 1024 * 6, NULL, 3, NULL);

    xTaskCreate(Bluetooth_Core, "Bluetooth Core Task", 1024 * 7, NULL, 1, NULL);

    /* High priority. This is the main backlight controller task*/
    xTaskCreate(TvBacklight_Task, "Backlight Manager Task", 1024 * 6, NULL, 4, NULL);

    /* Temporary while 1 loop for testing */
    // while(1)
    // {

    //     //TvBacklight_SetLED_Color(0, color);
    //     Set_TvBacklight_Strip_To_Constant_Color(color);
    //     Refresh_TvBacklight_Strip();

    //     color.blue -= 1;
    //     color.red += 1;

    //     vTaskDelay(pdMS_TO_TICKS(50));
    // }
}


void vSleepWithBluetooth(void)
{

}
