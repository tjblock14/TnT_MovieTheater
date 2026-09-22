#ifndef BLUETOOTH_CORE_H
#define BLUETOOTH_CORE_H

void Bluetooth_Core(void *pvParameters);

/*****************************************************
 * Configuration
 *****************************************************/


/*****************************************************
 * Defninitions
 *****************************************************/

 /* Application Profile. This is a way to organize the bluetooth server code */
 #define BACKLIGHT_PROFILE_NUM                (1)
 #define BACKLIGHT_PROFILE_APP_INDEX          (0)
 #define BACKLIGHT_APP_ID                     (0x55)

/*****************************************************
 * Bluetooth Enumerations
 *****************************************************/

 /* This is for the bluetooth attribute table. This is a list of data and controls that our device
  * exposes to connected devices. This enum gives each row an index */
enum BacklightServices
{
    /* Identify the group */
    BACKLIGHT_SERVICE = 0,

    /* Power (On/Off) */
    BACKLIGHT_POWER_CHAR,
    BACKLIGHT_POWER_VAL,

    /* Brightness */
    BACKLIGHT_BRIGHTNESS_CHAR,
    BACKLIGHT_BRIGHTNESS_VAL,

    BACKLIGHT_CONTROLS_MAX
};

/*****************************************************
 * Bluetooth Definitions
 *****************************************************/

 /* Discovery name */
#define BACKLIGHT_CONTROLLER_NAME "Backlight Controller"

#endif // BLUETOOTH_CORE_H