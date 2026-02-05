idf_component_register(SRCS "main.c"
                       PRIV_REQUIRES esp_driver_gpio fatfs nvs_flash
                                     cmd_system cmd_wifi cmd_nvs cli-api
                       INCLUDE_DIRS ".")
