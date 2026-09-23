idf_component_register(SRCS "network_events.c" "udp_raw_receiver.c" "wifi.c"
                    INCLUDE_DIRS "."
                    PRIV_REQUIRES 
                        Core
                        TheaterDevices
                        esp_wifi 
                        esp_event 
                        esp_netif 
                        nvs_flash)