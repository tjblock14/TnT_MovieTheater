idf_component_register(SRCS "TheaterManager.c" "main.c"
                    INCLUDE_DIRS "."
                    PRIV_REQUIRES
                        Wifi
                        TheaterDevices
                        Core
                        Bluetooth
                        nvs_flash
                        led_strip
                        esp_driver_gpio)