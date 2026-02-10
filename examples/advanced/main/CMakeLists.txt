idf_component_register(SRCS "main.c"
                       INCLUDE_DIRS "."
                       REQUIRES cli-api esp_driver_gpio
                                     cmd_system cmd_wifi cmd_nvs)
