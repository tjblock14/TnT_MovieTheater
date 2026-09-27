#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_bt.h"

#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_bt_defs.h"
#include "esp_bt_main.h"
#include "esp_bt_main.h"
#include "esp_gatt_common_api.h"

#include "BluetoothCore.h"
#include "BluetoothStructs.h"
#include "TvBacklightManager.h"

/* Tag used for esp logs */
static const char *TAG = "Bluetooth Core";

/* Local Functions */
void InitializeCharacteristics(BluetoothDevice_t *Device);
void InitializeAttributeTable(void);
void CheckBluetoothInit(void);
void TryStartInitialAdvertising(void);
void vSetUpAdvertisingData(void);

static void GapEventHandler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param);
static void GattProfileEventHandler(esp_gatts_cb_event_t event, esp_gatt_if_t gatt_interface_id, esp_ble_gatts_cb_param_t *param);
static void GattEventHandler(esp_gatts_cb_event_t event, esp_gatt_if_t interface_id, esp_ble_gatts_cb_param_t *param);

BluetoothDevice_t BluetoothStatus;

/* We want to use a Primary Service UUID */
static const uint16_t PrimaryServiceUuidType = ESP_GATT_UUID_PRI_SERVICE;

/* Tells the stack where to find the type identifier when it is a characteristic */
static const uint16_t CharacteristicUuidType = ESP_GATT_UUID_CHAR_DECLARE;

/* Bluetooth Task */
void Bluetooth_Core(void *pvParameters)
{
    esp_err_t err;

    BluetoothStatus.Characteristics.BacklightServiceStarted     = false;
    BluetoothStatus.Characteristics.InitialAdvertisingRequested = false;

    /* Initialize NVS */
    err = nvs_flash_init();

    if((err == ESP_ERR_NVS_NO_FREE_PAGES) || (err == ESP_ERR_NVS_NEW_VERSION_FOUND))
    {
        ESP_ERROR_CHECK(nvs_flash_erase());

        err = nvs_flash_init();
    }

    ESP_ERROR_CHECK(err);

    /* Initialize the array of GATT applications */
    BluetoothStatus.Characteristics.BacklightProfileTable[BACKLIGHT_PROFILE_APP_INDEX].Callback    = GattProfileEventHandler; // This is the function that handles our backlight GATT events
    BluetoothStatus.Characteristics.BacklightProfileTable[BACKLIGHT_PROFILE_APP_INDEX].InterfaceId = ESP_GATT_IF_NONE;

    InitializeCharacteristics(&BluetoothStatus);
    vSetUpAdvertisingData();

    /* Verify that all intiailization was successful */
    CheckBluetoothInit();

    /* The Bluetooth stack will now manage events. We are done here*/
    vTaskDelete(NULL);
}


/********************************************************************************************
 * FUNCTION : GattProfileEventHandler
 * PURPOSE  : Handles bluetooth GAP events such as edvertising starting, stopping, and
 *            pairing/security events.
 * INPUTS   :
 *      event             : The bluetooth GATT event that occurred
 *      gatt_interface_id : GATT application interface the event is from
 *      param             : Details of the event and bytes written by the client
 ********************************************************************************************/
static void GattProfileEventHandler(esp_gatts_cb_event_t event, esp_gatt_if_t gatt_interface_id, esp_ble_gatts_cb_param_t *param)
{
    esp_err_t err;

    switch (event)
    {
        case ESP_GATTS_REG_EVT:
            /* GATT Server application is being registered. Start by setting the name of the application and defaulting our flags */
            err = esp_ble_gap_set_device_name(BACKLIGHT_CONTROLLER_NAME);
            if(err != ESP_OK)
            {
                ESP_LOGE(TAG, "Failed to set device name, error code = %x", err);
            }

            BluetoothStatus.uAdvertisingSetupFlags.Bit.AdvertisingDone  = false;
            BluetoothStatus.uAdvertisingSetupFlags.Bit.ScanResponseDone = false;

            /* Configure the advertising packet */
            err = esp_ble_gap_config_adv_data(&BluetoothStatus.AdvertisingData);
            if(err != ESP_OK)
            {
                ESP_LOGE(TAG, "Advertising configuration request failed: %x", err);
                break;
            }

            /* Configure the scan response data */
            err = esp_ble_gap_config_adv_data(&BluetoothStatus.ScanResponseData);
            if(err != ESP_OK)
            {
                ESP_LOGE(TAG, "Scan response configuration request failed: %x", err);
                break;
            }

            /* Create the attribute table */
            err = esp_ble_gatts_create_attr_tab(BluetoothStatus.AttributeTable, gatt_interface_id, BACKLIGHT_CONTROLS_MAX, 0);
            if(err != ESP_OK)
            {
                ESP_LOGE(TAG, "create attr table failed, error code = %x", err);
            }
       	    break;

        case ESP_GATTS_READ_EVT:
            /* We received a read request from a client */
            esp_gatt_rsp_t Response      = {0};
            esp_gatt_status_t GattStatus = ESP_GATT_OK;
            uint8_t u8ReadValue;

            /* Retrieve the requested value */
            if(param->read.handle == BluetoothStatus.Characteristics.BacklightHandles[BACKLIGHT_POWER_VAL])
            {
                u8ReadValue = MainTvBacklightStrip.bPoweredOn;
            }
            else if(param->read.handle == BluetoothStatus.Characteristics.BacklightHandles[BACKLIGHT_BRIGHTNESS_VAL])
            {
                u8ReadValue = MainTvBacklightStrip.u8BrightnessPct;
            }
            else
            {
                /* Other attributes use automatic responses. */
                break;
            }

            /* Attach the handle for the attribute requested and get its memory location */
            Response.attr_value.handle = param->read.handle;
            Response.attr_value.offset = param->read.offset;

            /* Each setting contains exactly one byte (so far). We should not go past the one byte requested */
            if(param->read.offset > 1)
            {
                GattStatus = ESP_GATT_INVALID_OFFSET;
            }
            else if(param->read.offset == 0)
            {
                Response.attr_value.len = 1;
                Response.attr_value.value[0] = u8ReadValue;
            }  /* Offset 1 is the end of the value: return zero bytes if offset = 1 */

            /* Send the return value through GATT to the device that requested the information */
            err = esp_ble_gatts_send_response(gatt_interface_id, param->read.conn_id, param->read.trans_id, GattStatus, (GattStatus == ESP_GATT_OK ? &Response : NULL));
            if(err != ESP_OK)
            {
                ESP_LOGE(TAG, "Failed to send read response: %x", err);
            }
            break;

        case ESP_GATTS_WRITE_EVT:
            /* We received a write request from a client */
            esp_gatt_status_t WriteStatus = ESP_GATT_OK;

            /* Identify if any valid setting is being written */
            BluetoothStatus.WriteFlags.Bit.Power      = (param->write.handle == BluetoothStatus.Characteristics.BacklightHandles[BACKLIGHT_POWER_VAL]);
            BluetoothStatus.WriteFlags.Bit.Brightness = (param->write.handle == BluetoothStatus.Characteristics.BacklightHandles[BACKLIGHT_BRIGHTNESS_VAL]);

            if(!BluetoothStatus.WriteFlags.All)
            {
                /* No valid setting is being written, go no further */
                break;
            }

            /* As of today (9/19/26), all valid settings are only 1 byte. Verify this */
            if(param->write.is_prep)
            {
                WriteStatus = ESP_GATT_REQ_NOT_SUPPORTED;
            }
            else if(param->write.offset != 0)
            {
                WriteStatus = ESP_GATT_INVALID_OFFSET;
            }
            else if(param->write.len != 1)
            {
                WriteStatus = ESP_GATT_INVALID_ATTR_LEN;
            }
            else if(BluetoothStatus.WriteFlags.Bit.Power)
            {
                if (param->write.value[0] > 1)
                {
                    WriteStatus = ESP_GATT_OUT_OF_RANGE;
                }
                else
                {
                    MainTvBacklightStrip.bPoweredOn = (param->write.value[0] != 0);
                }
            }
            else // Writing brightness
            {
                if (param->write.value[0] > 100)
                {
                    WriteStatus = ESP_GATT_OUT_OF_RANGE;
                }
                else
                {
                    MainTvBacklightStrip.u8BrightnessPct = param->write.value[0];
                }
            }

            /* Reply only when the client requested a response. */
            if (param->write.need_rsp)
            {
                err = esp_ble_gatts_send_response(gatt_interface_id, param->write.conn_id, param->write.trans_id, WriteStatus, NULL);

                if (err != ESP_OK)
                {
                    ESP_LOGE(TAG, "Failed to send write response: %x", err);
                }
            }
            break;

        case ESP_GATTS_EXEC_WRITE_EVT:
            /* We received a write exectution request from a client*/
            err = esp_ble_gatts_send_response(gatt_interface_id, param->exec_write.conn_id, param->exec_write.trans_id, ESP_GATT_REQ_NOT_SUPPORTED, NULL);
            if(err != ESP_OK)
            {
                ESP_LOGE(TAG, "Failed to send execute-write response: %x", err);
            }
            break;

        case ESP_GATTS_MTU_EVT:
            /* We received a MTU configuration request from a client */
            ESP_LOGI(TAG, "ESP_GATTS_MTU_EVT, MTU %d", param->mtu.mtu);
            break;

        case ESP_GATTS_CONF_EVT:
            /* We received confirmation from the Client */
            ESP_LOGI(TAG, "ESP_GATTS_CONF_EVT, status = %d, attr_handle %d", param->conf.status, param->conf.handle);
            break;

        case ESP_GATTS_START_EVT:
            /* Verify that the service successfully started */
            if (param->start.status != ESP_GATT_OK)
            {
                ESP_LOGE(TAG, "Backlight service failed to start: %x", param->start.status);
                break;
            }

            BluetoothStatus.Characteristics.BacklightServiceStarted = true;
            TryStartInitialAdvertising();

            ESP_LOGI(TAG, "Backlight service started, handle: %u", (unsigned)param->start.service_handle);
            break;

        case ESP_GATTS_CONNECT_EVT:
            ESP_LOGI(TAG, "ESP_GATTS_CONNECT_EVT, conn_id = %d", param->connect.conn_id);
            ESP_LOG_BUFFER_HEX(TAG, param->connect.remote_bda, 6);
            esp_ble_conn_update_params_t conn_params = {0};
            memcpy(conn_params.bda, param->connect.remote_bda, sizeof(esp_bd_addr_t));
            /* For the iOS system, please refer to Apple official documents about the BLE connection parameters restrictions. */
            conn_params.latency = 0;
            conn_params.max_int = 0x20;    // max_int = 0x20*1.25ms = 40ms
            conn_params.min_int = 0x10;    // min_int = 0x10*1.25ms = 20ms
            conn_params.timeout = 400;     // timeout = 400*10ms    = 4000ms
            //start sent the update connection parameters to the peer device.
            esp_ble_gap_update_conn_params(&conn_params);
            break;

        case ESP_GATTS_DISCONNECT_EVT:
            ESP_LOGI(TAG, "ESP_GATTS_DISCONNECT_EVT, reason = 0x%x", param->disconnect.reason);
            esp_ble_gap_start_advertising(&BluetoothStatus.AdvertisingParameters);
            break;

        case ESP_GATTS_CREAT_ATTR_TAB_EVT:
            /* Verify that the attribute table creation was successful */
            if(param->add_attr_tab.status != ESP_GATT_OK)
            {
                ESP_LOGE(TAG, "Attribute table creation failed: %x", param->add_attr_tab.status);
                break;
            }

            /* Verify that all attributes were created */
            if(param->add_attr_tab.num_handle != BACKLIGHT_CONTROLS_MAX)
            {
                ESP_LOGE(TAG, "Unexpected attribute handle count: %d", param->add_attr_tab.num_handle);
                break;
            }

            /* Save the attribute handles */
            memcpy(BluetoothStatus.Characteristics.BacklightHandles, param->add_attr_tab.handles, sizeof(BluetoothStatus.Characteristics.BacklightHandles));

            /* Tell bluetooth to activate our service and verify that it was successful */
            err = esp_ble_gatts_start_service(BluetoothStatus.Characteristics.BacklightHandles[BACKLIGHT_SERVICE]);
            if(err != ESP_OK)
            {
                ESP_LOGE(TAG, "Service start request failed: %x", err);
            }

            break;

        case ESP_GATTS_STOP_EVT:
        case ESP_GATTS_OPEN_EVT:
        case ESP_GATTS_CANCEL_OPEN_EVT:
        case ESP_GATTS_CLOSE_EVT:
        case ESP_GATTS_LISTEN_EVT:
        case ESP_GATTS_CONGEST_EVT:
        case ESP_GATTS_UNREG_EVT:
        case ESP_GATTS_DELETE_EVT:
        default:
            break;
    }
}


/********************************************************************************************
 * FUNCTION : GapEventHandler
 * PURPOSE  : Handles bluetooth GAP events. GAP events handle how devices find and connect 
 *            to one another
 * INPUTS   :
 *      event    : The bluetooth GAP event that occurred
 *      param    : The details of the event
 ********************************************************************************************/
static void GapEventHandler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
    switch(event)
    {
        case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
            /* Advertising start completed. Check if it was successful */
            if (param->adv_start_cmpl.status == ESP_BT_STATUS_SUCCESS)
            {
                ESP_LOGI(TAG, "Advertising started");
            }
            else
            {
                BluetoothStatus.Characteristics.InitialAdvertisingRequested = false;
                ESP_LOGE(TAG, "Advertising failed: %x", param->adv_start_cmpl.status);
            }
            break;

        case ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT:
            /* Setting advertising data completed. Check if it was successful */
            if (param->adv_data_cmpl.status != ESP_BT_STATUS_SUCCESS)
            {
                ESP_LOGE(TAG, "Advertising data configuration failed");
                break;
            }

            BluetoothStatus.uAdvertisingSetupFlags.Bit.AdvertisingDone = true;
            TryStartInitialAdvertising();
            ESP_LOGI(TAG, "Advertising data ready");
            break;

        case ESP_GAP_BLE_SCAN_RSP_DATA_SET_COMPLETE_EVT:
            /* Setting response data completed. Check if it was successful */
            if (param->scan_rsp_data_cmpl.status != ESP_BT_STATUS_SUCCESS)
            {
                ESP_LOGE(TAG, "Scan response configuration failed");
                break;
            }

            BluetoothStatus.uAdvertisingSetupFlags.Bit.ScanResponseDone = true;
            TryStartInitialAdvertising();
            ESP_LOGI(TAG, "Scan response data ready");
            break;

        default:
            break;
    }
}


/********************************************************************************************
 * FUNCTION : CheckBluetoothInit
 * PURPOSE  : Performs a majority of the bluetooth stack initialization and checks to be
 *            sure that they were successful.
 ********************************************************************************************/
void CheckBluetoothInit(void)
{
    esp_err_t err;
    esp_bt_controller_config_t bt_config = BT_CONTROLLER_INIT_CONFIG_DEFAULT();

    err = esp_bt_controller_init(&bt_config);
    if(err)
    {
        ESP_LOGE(TAG, "Controller initialization failed");
        return;
    }

    err = esp_bt_controller_enable(ESP_BT_MODE_BLE);
    if(err)
    {
        ESP_LOGE(TAG, "Controller enable failed");
        return;
    }

    err = esp_bluedroid_init();
    if(err)
    {
        ESP_LOGE(TAG, "Bluethooth init failed");
        return;
    }

    err = esp_bluedroid_enable();
    if(err)
    {
        ESP_LOGE(TAG, "Bluetooth enable failed");
        return;
    }

    /* Set the GATT handler function */
    err = esp_ble_gatts_register_callback(GattEventHandler);
    if(err)
    {
        ESP_LOGE(TAG, "Error registering the GATT callback function. Error code: %x", err);
        return;
    }

    /* Set the GAP handler function */
    err = esp_ble_gap_register_callback(GapEventHandler);
    if (err)
    {
        ESP_LOGE(TAG, "Error registering the GAP callback function. Error code = %x", err);
        return;
    }

    /* Register the Backlight application */
    err = esp_ble_gatts_app_register(BACKLIGHT_APP_ID);
    if (err)
    {
        ESP_LOGE(TAG, "gatts app register error. Error codE: %x", err);
        return;
    }

    esp_err_t local_mtu_ret = esp_ble_gatt_set_local_mtu(512);
    if (local_mtu_ret)
    {
        ESP_LOGE(TAG, "Set local MTU failed. Error code; %x", local_mtu_ret);
    }
}

/********************************************************************************************
 * FUNCTION : GattEventHandler
 * PURPOSE  : Handles GATT bluetooth events. GATT bluetooth events handle how devices share
 *            information once they are connected to each other. This function receives events
 *            from the bluetooth stack and passes them to our handler
 * INPUTS   :
 *      event        : The event that occurred
 *      interface_id : Which GATT application is this event meant for?
 *      param        : Extra event-specific details and information
 ********************************************************************************************/
static void GattEventHandler(esp_gatts_cb_event_t event, esp_gatt_if_t interface_id, esp_ble_gatts_cb_param_t *param)
{
    /* Retrieve the backlight application struct */
    gatt_application_t *application = &BluetoothStatus.Characteristics.BacklightProfileTable[BACKLIGHT_PROFILE_APP_INDEX];

    if(event == ESP_GATTS_REG_EVT)
    {
        /* Ignore registration results for other applications. */
        if (param->reg.app_id != BACKLIGHT_APP_ID)
        {
            return;
        }

        /* If there was an issue registering the application, stop and log the error */
        if (param->reg.status != ESP_GATT_OK)
        {
            ESP_LOGE(TAG, "GATT application registration failed: %x", param->reg.status);
            return;
        }

        /* Save the application and interface IDs the bluetooth stack assigned */
        application->AppId = param->reg.app_id;
        application->InterfaceId = interface_id;
    }

    /* If registration has not completed yet, no need to forward events */
    if (application->InterfaceId == ESP_GATT_IF_NONE)
    {
        return;
    }

    /* If we got here, forward the event to our application */
    if (   (interface_id == application->InterfaceId)
        || (interface_id == ESP_GATT_IF_NONE))
    {
        if (application->Callback != NULL)
        {
            application->Callback(event, interface_id, param);
        }
    }
}

/********************************************************************************************
 * FUNCTION : TryStartInitialAdvertising
 * PURPOSE  : 
 ********************************************************************************************/
void TryStartInitialAdvertising(void)
{
    if (   (!BluetoothStatus.uAdvertisingSetupFlags.Bit.AdvertisingDone)
        || (!BluetoothStatus.uAdvertisingSetupFlags.Bit.ScanResponseDone)
        || (!BluetoothStatus.Characteristics.BacklightServiceStarted)
        || (BluetoothStatus.Characteristics.InitialAdvertisingRequested))
    {
        return;
    }

    esp_err_t err = esp_ble_gap_start_advertising(&BluetoothStatus.AdvertisingParameters);

    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Advertising start request failed: %x", err);
        return;
    }

    BluetoothStatus.Characteristics.InitialAdvertisingRequested = true;
}

/********************************************************************************************
 * FUNCTION : InitializeCharacteristics
 * PURPOSE  : 
 ********************************************************************************************/
void InitializeCharacteristics(BluetoothDevice_t *Device)
{
    const uint8_t BacklightServiceUuid[16] =
    {
        0xF9, 0xC8, 0x98, 0xEF, 0x42, 0x47, 0x5C, 0x9C, 
        0x7C, 0x47, 0x33, 0x1C, 0xA8, 0x01, 0x11, 0x0F
    }; // 0f1101a8-1c33-477c-9c5c-4742ef98c8f9
    const uint8_t PowerUuid[16] =
    {
        0x64, 0x71, 0x32, 0xA4, 0xA8, 0xEA, 0x94, 0xA1,
        0x77, 0x4B, 0xD7, 0x4C, 0xD0, 0xA3, 0xF3, 0xEB
    }; // ebf3a3d0-4cd7-4b77-a194-eaa8a4327164
    const uint8_t BrightnessUuid[16] =
    {
        0xB5, 0x6D, 0xAC, 0x10, 0xC8, 0x2A, 0x66, 0xBF,
        0x99, 0x47, 0x2E, 0xB3, 0x9B, 0x27, 0xA4, 0x47
    }; // 47a4279b-b32e-4799-bf66-2ac810ac6db5

    /* Copy all of the UUiDs into their location in the main bluetooth struct */
    memcpy(Device->Characteristics.BacklightService.Uuid, BacklightServiceUuid, sizeof(BacklightServiceUuid));
    memcpy(Device->Characteristics.Power.Uuid, PowerUuid, sizeof(PowerUuid));
    memcpy(Device->Characteristics.Brightness.Uuid, BrightnessUuid, sizeof(BrightnessUuid));

    /* Power and Brightness support reads and writes */
    Device->Characteristics.Power.Permissions      = (ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_WRITE);
    Device->Characteristics.Brightness.Permissions = (ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_WRITE);

    InitializeAttributeTable();
}

/********************************************************************************************
 * FUNCTION : InitializeAttributeTable
 * PURPOSE  : 
 ********************************************************************************************/
void InitializeAttributeTable(void)
{
    /* Backlight Service */
    BluetoothStatus.AttributeTable[BACKLIGHT_SERVICE].attr_control.auto_rsp = ESP_GATT_AUTO_RSP;
    BluetoothStatus.AttributeTable[BACKLIGHT_SERVICE].att_desc.uuid_length  = ESP_UUID_LEN_16;
    BluetoothStatus.AttributeTable[BACKLIGHT_SERVICE].att_desc.uuid_p       = (uint8_t *)&PrimaryServiceUuidType;
    BluetoothStatus.AttributeTable[BACKLIGHT_SERVICE].att_desc.perm         = ESP_GATT_PERM_READ;
    BluetoothStatus.AttributeTable[BACKLIGHT_SERVICE].att_desc.max_length   = sizeof(BluetoothStatus.Characteristics.BacklightService.Uuid);
    BluetoothStatus.AttributeTable[BACKLIGHT_SERVICE].att_desc.length       = sizeof(BluetoothStatus.Characteristics.BacklightService.Uuid);
    BluetoothStatus.AttributeTable[BACKLIGHT_SERVICE].att_desc.value        = (uint8_t *)&BluetoothStatus.Characteristics.BacklightService.Uuid;

    /* Power Characteristic */
    BluetoothStatus.AttributeTable[BACKLIGHT_POWER_CHAR].attr_control.auto_rsp = ESP_GATT_AUTO_RSP;
    BluetoothStatus.AttributeTable[BACKLIGHT_POWER_CHAR].att_desc.uuid_length  = ESP_UUID_LEN_16;
    BluetoothStatus.AttributeTable[BACKLIGHT_POWER_CHAR].att_desc.uuid_p       = (uint8_t *)&CharacteristicUuidType;
    BluetoothStatus.AttributeTable[BACKLIGHT_POWER_CHAR].att_desc.perm         = ESP_GATT_PERM_READ;
    BluetoothStatus.AttributeTable[BACKLIGHT_POWER_CHAR].att_desc.max_length   = sizeof(BluetoothStatus.Characteristics.Power.Permissions);
    BluetoothStatus.AttributeTable[BACKLIGHT_POWER_CHAR].att_desc.length       = sizeof(BluetoothStatus.Characteristics.Power.Permissions);
    BluetoothStatus.AttributeTable[BACKLIGHT_POWER_CHAR].att_desc.value        = (uint8_t *)&BluetoothStatus.Characteristics.Power.Permissions;

    /* Power Value */
    BluetoothStatus.AttributeTable[BACKLIGHT_POWER_VAL].attr_control.auto_rsp = ESP_GATT_RSP_BY_APP;
    BluetoothStatus.AttributeTable[BACKLIGHT_POWER_VAL].att_desc.uuid_length  = ESP_UUID_LEN_128;
    BluetoothStatus.AttributeTable[BACKLIGHT_POWER_VAL].att_desc.uuid_p       = BluetoothStatus.Characteristics.Power.Uuid;
    BluetoothStatus.AttributeTable[BACKLIGHT_POWER_VAL].att_desc.perm         = (ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE); // Validate the request before accepting
    BluetoothStatus.AttributeTable[BACKLIGHT_POWER_VAL].att_desc.max_length   = sizeof(MainTvBacklightStrip.bPoweredOn);
    BluetoothStatus.AttributeTable[BACKLIGHT_POWER_VAL].att_desc.length       = sizeof(MainTvBacklightStrip.bPoweredOn);
    BluetoothStatus.AttributeTable[BACKLIGHT_POWER_VAL].att_desc.value        = (uint8_t *)&MainTvBacklightStrip.bPoweredOn;

    /* Brightness Characteristic */
    BluetoothStatus.AttributeTable[BACKLIGHT_BRIGHTNESS_CHAR].attr_control.auto_rsp = ESP_GATT_AUTO_RSP;
    BluetoothStatus.AttributeTable[BACKLIGHT_BRIGHTNESS_CHAR].att_desc.uuid_length  = ESP_UUID_LEN_16;
    BluetoothStatus.AttributeTable[BACKLIGHT_BRIGHTNESS_CHAR].att_desc.uuid_p       = (uint8_t *)&CharacteristicUuidType;
    BluetoothStatus.AttributeTable[BACKLIGHT_BRIGHTNESS_CHAR].att_desc.perm         = ESP_GATT_PERM_READ; // Validate the request before accepting
    BluetoothStatus.AttributeTable[BACKLIGHT_BRIGHTNESS_CHAR].att_desc.max_length   = sizeof(BluetoothStatus.Characteristics.Brightness.Permissions);
    BluetoothStatus.AttributeTable[BACKLIGHT_BRIGHTNESS_CHAR].att_desc.length       = sizeof(BluetoothStatus.Characteristics.Brightness.Permissions);
    BluetoothStatus.AttributeTable[BACKLIGHT_BRIGHTNESS_CHAR].att_desc.value        = (uint8_t *)&BluetoothStatus.Characteristics.Brightness.Permissions;

    /* Brightness Characteristic Value */
    BluetoothStatus.AttributeTable[BACKLIGHT_BRIGHTNESS_VAL].attr_control.auto_rsp = ESP_GATT_RSP_BY_APP;
    BluetoothStatus.AttributeTable[BACKLIGHT_BRIGHTNESS_VAL].att_desc.uuid_length  = ESP_UUID_LEN_128;
    BluetoothStatus.AttributeTable[BACKLIGHT_BRIGHTNESS_VAL].att_desc.uuid_p       = BluetoothStatus.Characteristics.Brightness.Uuid;
    BluetoothStatus.AttributeTable[BACKLIGHT_BRIGHTNESS_VAL].att_desc.perm         = (ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE);
    BluetoothStatus.AttributeTable[BACKLIGHT_BRIGHTNESS_VAL].att_desc.max_length   = sizeof(MainTvBacklightStrip.u8BrightnessPct);
    BluetoothStatus.AttributeTable[BACKLIGHT_BRIGHTNESS_VAL].att_desc.length       = sizeof(MainTvBacklightStrip.u8BrightnessPct);
    BluetoothStatus.AttributeTable[BACKLIGHT_BRIGHTNESS_VAL].att_desc.value        = &MainTvBacklightStrip.u8BrightnessPct;
}

/********************************************************************************************
 * FUNCTION : vSetUpAdvertisingData
 * PURPOSE  : Set up the advertising service the website will use
 ********************************************************************************************/
void vSetUpAdvertisingData(void)
{
    /* The advertising data */
    BluetoothStatus.AdvertisingData.set_scan_rsp     = false;
    BluetoothStatus.AdvertisingData.include_name     = false;
    BluetoothStatus.AdvertisingData.service_uuid_len = sizeof(BluetoothStatus.Characteristics.BacklightService.Uuid);
    BluetoothStatus.AdvertisingData.p_service_uuid   = (uint8_t *)BluetoothStatus.Characteristics.BacklightService.Uuid;
    BluetoothStatus.AdvertisingData.flag             = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT);

    /* Now the scan response data */
    BluetoothStatus.ScanResponseData.set_scan_rsp = true;
    BluetoothStatus.ScanResponseData.include_name = true;

    /* Advertising Parameters */
    BluetoothStatus.AdvertisingParameters.adv_int_min       = 0x100; // minimum advertising interval (0x100 * 0.625ms? = 160ms)
    BluetoothStatus.AdvertisingParameters.adv_int_max       = 0x200; // maximum advertising interval (0x200 * 0.655ms? = 320ms)
    BluetoothStatus.AdvertisingParameters.adv_type          = ADV_TYPE_IND; // Make the device scannable and connectable
    BluetoothStatus.AdvertisingParameters.own_addr_type     = BLE_ADDR_TYPE_PUBLIC; // Use the controller Bluetooth address
    BluetoothStatus.AdvertisingParameters.channel_map       = ADV_CHNL_ALL; // Advertise on all primary BLE advertising channels
    BluetoothStatus.AdvertisingParameters.adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY; // Allow scan and connectrion requests from any nearby devices
}