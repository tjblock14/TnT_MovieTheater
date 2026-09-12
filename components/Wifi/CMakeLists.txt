idf_component_register(SRCS "UdpRawReceiver.c" "wifi.c"
                    INCLUDE_DIRS "."
                    PRIV_REQUIRES 
                        Core
                        esp_wifi 
                        esp_event 
                        esp_netif 
                        nvs_flash)