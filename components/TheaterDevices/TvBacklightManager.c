#include "TvBacklightManager.h"
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

/* This will be the TV Backlight LED Strip object */
static led_strip_handle_t TvBacklightStrip = NULL;

/* FreeRTOS task and queue used for the tv backlight LEDs */
static QueueHandle_t q_TvBacklight_Queue = NULL;
static TaskHandle_t  t_TvBacklight_TaskHandle = NULL;

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
static void TvBacklight_Task(void *pv)
{
    TvBacklight_Init();

    /* Command that will be used within this task to set the LEDs */
    tv_backlight_cmd_with_info_t backlight_cmd;

    // FIXME: unsure why I need this yet
    const TickType_t frameDelay = pdMS_TO_TICKS(30);

    while(1)
    {
        /* Check the queue for any pending commands */
        while(xQueueReceive(q_TvBacklight_Queue, &backlight_cmd, 0) == pdTRUE)
        {
            switch(backlight_cmd.action)
            {
                case TV_BACKLIGHT_CMD_SOLID_STRIP:
                    Set_TvBacklight_Strip_To_Constant_Color(backlight_cmd.set_led.fill_strip.color);
                    break;

                case TV_BACKLIGHT_CMD_SINGLE_LED:
                    TvBacklight_SetLED_Color(1, backlight_cmd.set_led.one_led.color);
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

    if (TvBacklightStrip)
    {   
        /* The Tv Backlight strip has already been initializied, return OK */
        return err;
    }

    /* Create the LED strip object */
    err = led_strip_new_spi_device(&TvBacklight_Strip_Cfg, &spi_TvBacklight_Cfg, &TvBacklightStrip);

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
    if(!TvBacklightStrip || (index >= NUM_BACKLIGHT_LEDS))
    {
        return ESP_ERR_INVALID_STATE;
    }

    /* Set the color of the specified LED */
    esp_err_t err = led_strip_set_pixel_rgbw(
        TvBacklightStrip,
        index,
        color.red,
        color.green,
        color.blue,
        color.white
    );

    if(err == ESP_OK)
    {
        /* Refresh the strip so it actually updates */
        Refresh_TvBacklight_Strip();
    }

    if(err == ESP_OK)
    {
        ESP_LOGI(TAG, "Set color to R:%d, G:%d, B:%d, W:%d", color.red, color.green, color.blue, color.white);
    }
    else
    {
        ESP_LOGE(TAG, "Error: %d", err);
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
    if(!TvBacklightStrip)
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
    if(!TvBacklightStrip)
    {
        return ESP_ERR_INVALID_STATE;
    }

    /* Refresh the LED strip*/
    esp_err_t err = led_strip_refresh(TvBacklightStrip);

    return err;
}

/**************************************************************************
 * Purpose: Clear the whole LED strip
 * Returns: 
 *      The result of sending the clear command to the strip
 **************************************************************************/
esp_err_t Clear_TvBacklight_Strip(void)
{
    if(!TvBacklightStrip)
    {
        return ESP_ERR_INVALID_STATE;
    }

    /* Refresh the LED strip*/
    esp_err_t err = led_strip_clear(TvBacklightStrip);

    return err;
}