#include "TvBacklightManager.h"
#include "BluetoothCore.h"
#include "esp_log.h"
#include "esp_err.h"
#include "CoreDefines.h"
#include "led_strip.h"
#include "led_strip_spi.h"
#include "driver/gpio.h"

#ifdef DEBUG_CFG
    /* Tag identifier for esp logs */
    static const char *TAG = "TV BACKLIGHT";
#endif

/* Local Functions */
void vInitLedStripHw(void);
void PowerBacklightStripOn(void);
void PowerBacklightStripOff(void);

/* FreeRTOS task and queue used for the tv backlight LEDs */
QueueHandle_t TvBacklight_Queue             = NULL;

BacklightStrip_t MainTvBacklightStrip;

/* Configuration for the LED Strip*/
led_strip_config_t TvBacklight_Strip_Cfg = 
{
    .strip_gpio_num         = TV_BACKLIGHT_PIN,
    .max_leds               = NUM_BACKLIGHT_LEDS,
    .led_model              = LED_MODEL_SK6812,
    .color_component_format = LED_STRIP_COLOR_COMPONENT_FMT_GRBW,
    .flags.invert_out       = false
};

led_strip_spi_config_t spi_TvBacklight_Cfg =
{
    .spi_bus = SPI2_HOST,
    .clk_src = SPI_CLK_SRC_DEFAULT,
    .flags.with_dma = true // for optimal performance
};

/**************************************************************************
 * Purpose: This is the FreeRTOS task for the TV backlight LED strip 
 **************************************************************************/
void TvBacklight_Task(void *pv)
{
    TvBacklight_Init();

    /* Command that will be used within this task to set the LEDs */
    tv_backlight_cmd_with_info_t backlight_cmd;

    while(1)
    {
        /* Check the queue for any pending commands. Block this task here until we receive a command by using portMAX_DELAY */
        while(xQueueReceive(TvBacklight_Queue, &backlight_cmd, portMAX_DELAY) == pdTRUE)
        {
            switch(backlight_cmd.action)
            {
                case TV_BACKLIGHT_CMD_POWER_ON:
                    PowerBacklightStripOn();
                    break;

                case TV_BACKLIGHT_CMD_POWER_DOWN:
                    PowerBacklightStripOff();
                    break;

                case TV_BACKLIGHT_CMD_SET_BRIGHTNESS:
                    MainTvBacklightStrip.u8BrightnessPct = backlight_cmd.Data.BrightnessPct;
                    break;

                case TV_BACKLIGHT_CMD_SOLID_STRIP:
                    Set_TvBacklight_Strip_To_Constant_Color(backlight_cmd.Data.fill_strip.color);
                    break;

                case TV_BACKLIGHT_CMD_SINGLE_LED:
                    TvBacklight_SetLED_Color(backlight_cmd.Data.one_led.index, backlight_cmd.Data.one_led.color);
                    
                    #ifdef DEBUG_CFG
                       // ESP_LOGE(TAG, "Command received");
                    #endif
                    break;

                case TV_BACKLIGHT_CMD_REFRESH:
                    esp_err_t err = Refresh_TvBacklight_Strip();
                    break;

                default:
                    break;
            }
        }

    }
}

/**************************************************************************
 * Purpose: This Function is responsible for initializing the LED Strip 
 *          device for this project. It simply populates the strip object
 * Returns: 
 *      err: Result of the attempt to initialize the tv backlight strip
 **************************************************************************/
esp_err_t TvBacklight_Init(void)
{
    esp_err_t err = ESP_OK;

    /* Initialize the hardware for the LED Strip */
    vInitLedStripHw();

    if (MainTvBacklightStrip.Object)
    {   
        /* The Tv Backlight strip has already been initializied, return OK */
        return err;
    }

    /* Create the LED strip object */
    err = led_strip_new_spi_device(&TvBacklight_Strip_Cfg, &spi_TvBacklight_Cfg, &MainTvBacklightStrip.Object);

    /* Default the global struct values*/
    MainTvBacklightStrip.bPoweredOn      = false;
    MainTvBacklightStrip.u8BrightnessPct = 100;

    /* Create the Backlight Command Queue and verify it was successfully created */
    TvBacklight_Queue = xQueueCreate(16, sizeof(tv_backlight_cmd_with_info_t));

    if(TvBacklight_Queue == NULL)
    {
        #ifdef DEBUG_CFG
        ESP_LOGE(TAG, "Failed to create the Backlight Command Queue");
        #endif
    }

    #ifdef DEBUG_CFG
        ESP_LOGI(TAG, "TV backlight strip is created on GPIO %d with %d LEDs", TV_BACKLIGHT_PIN, NUM_BACKLIGHT_LEDS);
    #endif

    return err;
}

/**************************************************************************
 * Purpose: This Function is called to set an LED to a specified color
 * Inputs:
 *      uint16_t index: The index of the LED on the strip
 *      backlight_led_values_t color: the color the LED should be set to
 * Returns: 
 *      The result of sending the set led command to the strip
 **************************************************************************/
esp_err_t TvBacklight_SetLED_Color(uint16_t index, backlight_led_values_t color)
{
    /* If the LED Strip object does not exist, or the index is greater than the number of LEDs on the strip, return an error */
    if(!MainTvBacklightStrip.Object || (index >= NUM_BACKLIGHT_LEDS))
    {
        return ESP_ERR_INVALID_STATE;
    }

    /* Set the color of the specified LED */
    esp_err_t err = led_strip_set_pixel_rgbw(
        MainTvBacklightStrip.Object,
        index,
        color.red,
        color.green,
        color.blue,
        color.white
    );

    if(err != ESP_OK)
    {
        #ifdef DEBUG_CFG
        ESP_LOGE(TAG, "Error: %d", err);
        #endif
    }

    return err;
}

/**************************************************************************
 * Purpose: This Function sets the whole TV Backlight strip to a specified color
 * Inputs:
 *      backlight_led_values_t color: the color the strip should be set to
 * Returns: 
 *      The result of sending the set led command to the strip
 **************************************************************************/
esp_err_t Set_TvBacklight_Strip_To_Constant_Color(backlight_led_values_t color)
{
    /* Ensure the Backlight strip object exists */
    if(!MainTvBacklightStrip.Object)
    {
        return ESP_ERR_INVALID_STATE;
    }

    /* Set every LED on the strip to the specified color */
    for(int i = 0; i < NUM_BACKLIGHT_LEDS; i++)
    {
        esp_err_t err = TvBacklight_SetLED_Color(i, color);

        if(err != ESP_OK)
        {
            return err;
        }
    }

    /* Refresh the strip so it actually updates */
    Refresh_TvBacklight_Strip();

    /* If we got here, it means every LED was successfully set */
    return ESP_OK;
}

/**************************************************************************
 * Purpose: This Function refreshes the memory colors to the LEDs
 * Returns: 
 *      The result of sending the set led command to the strip
 **************************************************************************/
esp_err_t Refresh_TvBacklight_Strip(void)
{
    if(!MainTvBacklightStrip.Object)
    {
        return ESP_ERR_INVALID_STATE;
    }

    /* Refresh the LED strip*/
    esp_err_t err = led_strip_refresh(MainTvBacklightStrip.Object);

    return err;
}

/**************************************************************************
 * Purpose: Clear the whole LED strip
 * Returns: 
 *      The result of sending the clear command to the strip
 **************************************************************************/
esp_err_t Clear_TvBacklight_Strip(void)
{
    if(!MainTvBacklightStrip.Object)
    {
        return ESP_ERR_INVALID_STATE;
    }

    /* Refresh the LED strip*/
    esp_err_t err = led_strip_clear(MainTvBacklightStrip.Object);

    return err;
}

/**************************************************************************
 * FUNCTION : vInitLedStripHw
 * PURPOSE  : Configure the pin thaet controls the power to the LED strip
 **************************************************************************/
void vInitLedStripHw(void)
{
    gpio_config_t BacklightStripConfig = {
        .pin_bit_mask = GPIO_INPUT_PIN_SELECT,
        .mode         = GPIO_MODE_INPUT_OUTPUT, // Really it is an output, but set to both so we can read the status
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_ENABLE,
        .intr_type    = GPIO_INTR_DISABLE
    };

    gpio_config(&BacklightStripConfig);

    /* Enable the power to the strip on start up */
    PowerBacklightStripOn();
}

/**************************************************************************
 * FUNCTION : PowerBacklightStripOn
 * PURPOSE  : Turn on the pin that powers the backlight LED strip
 **************************************************************************/
void PowerBacklightStripOn(void)
{
    /* Enable the Power to the strip */
    ESP_ERROR_CHECK(gpio_set_level(BACKLIGHT_STRIP_ENABLE_PIN, true));

    /* Restore the data output without changing its SPI signal routing. */
    ESP_ERROR_CHECK(gpio_set_pull_mode(TV_BACKLIGHT_PIN, GPIO_FLOATING));
    ESP_ERROR_CHECK(gpio_set_direction(TV_BACKLIGHT_PIN, GPIO_MODE_OUTPUT));
}

/**************************************************************************
 * FUNCTION : PowerBacklightStripOff
 * PURPOSE  : Turn off the pin that powers the backlight LED strip
 **************************************************************************/
void PowerBacklightStripOff(void)
{
    /* Start by clearing the LED strip */
    Clear_TvBacklight_Strip();

    /* Stop driving data before removing power to avoid back-powering the strip. */
    ESP_ERROR_CHECK(gpio_set_direction(TV_BACKLIGHT_PIN, GPIO_MODE_INPUT));
    ESP_ERROR_CHECK(gpio_set_pull_mode(TV_BACKLIGHT_PIN, GPIO_PULLDOWN_ONLY));

    /* Then disable the GPIO pin that provides power to the strip. */
    ESP_ERROR_CHECK(gpio_set_level(BACKLIGHT_STRIP_ENABLE_PIN, false));

    /* If the device was turned off via bluetooth, set it for sleep */
    vSleepWithBluetooth();
}
