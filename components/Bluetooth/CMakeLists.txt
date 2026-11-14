idf_component_register(SRCS "BluetoothCore.c"
                    INCLUDE_DIRS "."
                    PRIV_REQUIRES
                        bt
                        nvs_flash
                        TheaterDevices
                        Core)