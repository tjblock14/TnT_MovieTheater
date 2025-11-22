idf_component_register(SRCS "bluetooth_core.c"
                    INCLUDE_DIRS "."
                    PRIV_REQUIRES
                        bt
                        nvs_flash)