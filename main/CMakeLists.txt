idf_component_register(SRCS "TheaterManager.c" "main.c"
                    INCLUDE_DIRS "."
                    PRIV_REQUIRES
                        Wifi
                        Core
                        nvs_flash)