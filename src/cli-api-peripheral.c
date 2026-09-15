/**
 * @file cli-api-peripheral.c
 * @author Pedro Luis Dionísio Fraga (pedrodfraga@hotmail.com)
 *
 * @brief Console peripheral bring-up (UART, USB CDC or USB Serial JTAG).
 *
 * Pure driver-level setup; does not touch `s_cli`.
 *
 * @version 0.1
 * @date 2026-02-05
 *
 * @copyright Copyright (c) 2026
 *
 */

#include <driver/uart.h>
#include <driver/uart_vfs.h>
#include <fcntl.h>
#include <sdkconfig.h>
#include <soc/soc_caps.h>
#include <stdio.h>
#include <unistd.h>

#include "cli-api-private.h"
#if SOC_USB_SERIAL_JTAG_SUPPORTED
#include <driver/usb_serial_jtag.h>
#include <driver/usb_serial_jtag_vfs.h>
#endif
#if CONFIG_ESP_CONSOLE_USB_CDC
#include <esp_vfs_cdcacm.h>
#endif

void cli_init_peripheral(void)
{
  /* Drain stdout before reconfiguring */
  fflush(stdout);
  fsync(fileno(stdout));

#if defined(CONFIG_ESP_CONSOLE_UART_DEFAULT) || defined(CONFIG_ESP_CONSOLE_UART_CUSTOM)
  /* UART: Configure line endings */
  uart_vfs_dev_port_set_rx_line_endings(CONFIG_ESP_CONSOLE_UART_NUM, ESP_LINE_ENDINGS_CR);
  uart_vfs_dev_port_set_tx_line_endings(CONFIG_ESP_CONSOLE_UART_NUM, ESP_LINE_ENDINGS_CRLF);

  /* Configure UART */
  const uart_config_t uart_config = {
    .baud_rate = CONFIG_ESP_CONSOLE_UART_BAUDRATE,
    .data_bits = UART_DATA_8_BITS,
    .parity = UART_PARITY_DISABLE,
    .stop_bits = UART_STOP_BITS_1,
#if SOC_UART_SUPPORT_REF_TICK
    .source_clk = UART_SCLK_REF_TICK,
#elif SOC_UART_SUPPORT_XTAL_CLK
    .source_clk = UART_SCLK_XTAL,
#endif
  };
  ESP_ERROR_CHECK(uart_driver_install(CONFIG_ESP_CONSOLE_UART_NUM, 256, 0, 0, NULL, 0));
  ESP_ERROR_CHECK(uart_param_config(CONFIG_ESP_CONSOLE_UART_NUM, &uart_config));
  uart_vfs_dev_use_driver(CONFIG_ESP_CONSOLE_UART_NUM);

#elif defined(CONFIG_ESP_CONSOLE_USB_CDC)
  /* USB CDC: Configure line endings */
  esp_vfs_dev_cdcacm_set_rx_line_endings(ESP_LINE_ENDINGS_CR);
  esp_vfs_dev_cdcacm_set_tx_line_endings(ESP_LINE_ENDINGS_CRLF);
  fcntl(fileno(stdout), F_SETFL, 0);
  fcntl(fileno(stdin), F_SETFL, 0);

#elif defined(CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG)
  /* USB Serial JTAG: Configure line endings */
  usb_serial_jtag_vfs_set_rx_line_endings(ESP_LINE_ENDINGS_CR);
  usb_serial_jtag_vfs_set_tx_line_endings(ESP_LINE_ENDINGS_CRLF);
  fcntl(fileno(stdout), F_SETFL, 0);
  fcntl(fileno(stdin), F_SETFL, 0);

  usb_serial_jtag_driver_config_t jtag_config = {
    .tx_buffer_size = 256,
    .rx_buffer_size = 256,
  };
  ESP_ERROR_CHECK(usb_serial_jtag_driver_install(&jtag_config));
  usb_serial_jtag_vfs_use_driver();

#else
#error Unsupported console type
#endif

  /* Disable buffering on stdin */
  setvbuf(stdin, NULL, _IONBF, 0);
}
