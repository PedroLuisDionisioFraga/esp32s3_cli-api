idf_component_register(SRCS "console_example_main.c" "console_settings.c"
                       PRIV_REQUIRES console esp_driver_uart esp_driver_gpio fatfs esp_driver_usb_serial_jtag
                                     nvs_flash cmd_system cmd_wifi cmd_nvs cli-api
                       INCLUDE_DIRS ".")
