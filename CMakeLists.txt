idf_component_register(SRCS "src/cli-api.c"
                            "src/cli-api-state.c"
                            "src/cli-api-storage.c"
                            "src/cli-api-peripheral.c"
                            "src/cli-api-console.c"
                            "src/cli-api-command.c"
                    INCLUDE_DIRS "include"
                    PRIV_REQUIRES console esp_driver_uart esp_driver_usb_serial_jtag fatfs nvs_flash)
