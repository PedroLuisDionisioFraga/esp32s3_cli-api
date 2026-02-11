idf_component_register(SRCS "components/cli-api/cli-api.c"
                    INCLUDE_DIRS "components/cli-api/include"
                    REQUIRES console esp_driver_uart esp_driver_usb_serial_jtag fatfs nvs_flash)
