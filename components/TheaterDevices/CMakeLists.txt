idf_component_register(SRCS "TvBacklightManager.c"
                    INCLUDE_DIRS "."
                    REQUIRES
                        led_strip
                    PRIV_REQUIRES 
                        Core
                        Bluetooth
                        driver
                        esp_driver_rmt 
                        )
