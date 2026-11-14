#ifndef BLUETOOTH_STRUCTS_H
#define BLUETOOTH_STRUCTS_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_gatts_api.h"
#include "esp_gap_ble_api.h"
#include "BluetoothCore.h"

/* Information for a GATT application profile */
typedef struct
{
    esp_gatts_cb_t       Callback;
    uint16_t             InterfaceId;
    uint16_t             AppId;
    uint16_t             ConnectionId;
    uint16_t             ServiceHandle;
    esp_gatt_srvc_id_t   ServiceId;
    uint16_t             CharacteristicHandle;
    esp_bt_uuid_t        CharacteristicUuid;
    esp_gatt_perm_t      Permissions;
    esp_gatt_char_prop_t AvailableOperations;
    uint16_t             DescriptorHandle;
    esp_bt_uuid_t        DescriptorUuid;
} gatt_application_t;

 /* Bluetooth Status Flags */
typedef union
{
    uint8_t All;

    struct
    {
        uint8_t AdvertisingDone  : 1; // Has the stack finished configuring the advertising packet
        uint8_t ScanResponseDone : 1; // Has the stack finished configuring the scan response packet
        uint8_t                  : 6; // Unused bits
    } Bit;
} BluetoothSetupFlags_t;

typedef union
{
    uint8_t All;

    struct
    {
        uint8_t Power      : 1;
        uint8_t Brightness : 1;
        uint8_t            : 6; // Unused
    } Bit;
} BacklightControllerBtWriteFlags_t;

/* One backlight characteristic */
typedef struct
{
    uint8_t Uuid[16];
    uint8_t Permissions;
} BacklightCharacteristic_t;

/* Hold Bluetooth characteristic details */
typedef struct
{
    bool BacklightServiceStarted;
    bool InitialAdvertisingRequested;

    BacklightCharacteristic_t BacklightService;
    BacklightCharacteristic_t Power;
    BacklightCharacteristic_t Brightness;

    uint16_t BacklightHandles[BACKLIGHT_CONTROLS_MAX];
    gatt_application_t BacklightProfileTable[BACKLIGHT_PROFILE_NUM];

} BtCharacteristics_t;

/* Bluetooth Status */
typedef struct
{
    esp_gatts_attr_db_t AttributeTable[BACKLIGHT_CONTROLS_MAX];
    BtCharacteristics_t Characteristics;
    
    BluetoothSetupFlags_t uAdvertisingSetupFlags;
    esp_ble_adv_data_t    AdvertisingData;
    esp_ble_adv_params_t  AdvertisingParameters;
    esp_ble_adv_data_t    ScanResponseData;

    /* Write Flags that tracks if a setting is being written */
    BacklightControllerBtWriteFlags_t WriteFlags;
} BluetoothDevice_t;

#endif // BLUETOOTH_STRUCTS_H