idf_component_register(SRCS "cli-api.c"
                    INCLUDE_DIRS "include"
                    REQUIRES console esp_driver_uart esp_driver_usb_serial_jtag esp_driver_gpio nvs_flash fatfs nvs_flash)
