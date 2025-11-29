idf_component_register(SRCS "TvBacklightManager.c"
                    INCLUDE_DIRS "."
                    PRIV_REQUIRES 
                        Core
                        driver
                        esp_driver_rmt 
                        led_strip
                        )