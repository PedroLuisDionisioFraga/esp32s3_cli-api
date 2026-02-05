| Supported Targets | ESP32 | ESP32-C2 | ESP32-C3 | ESP32-C5 | ESP32-C6 | ESP32-C61 | ESP32-H2 | ESP32-H21 | ESP32-H4 | ESP32-P4 | ESP32-S2 | ESP32-S3 |
| ----------------- | ----- | -------- | -------- | -------- | -------- | --------- | -------- | --------- | -------- | -------- | -------- | -------- |

# CLI-API - Simplified Console API for ESP32

A simplified, header-only like API for creating command-line interfaces (CLI) on ESP32 devices. This project wraps the complexity of ESP-IDF's console component and argtable3 library into an easy-to-use interface.

## Features

- **Simple Command Registration**: Define commands using declarative structs instead of complex argtable3 code
- **Multiple Argument Types**: Support for integers, strings, and boolean flags
- **Automatic Help Generation**: Built-in help system for all registered commands
- **Command History**: Persistent command history stored in flash memory
- **Multiple Interface Support**: UART, USB_SERIAL_JTAG, and USB_CDC
- **Example Commands**: Includes WiFi, NVS, and system management commands
- **Clean C API**: No extern "C" wrappers needed, pure C implementation

## Quick Example

```c
static int cmd_hello(int argc, char **argv)
{
  printf("Hello World! Welcome to ESP32 console!\n");
  return 0;
}

void app_main(void)
{
  cli_config_t cli_cfg = CLI_CONFIG_DEFAULT();
  cli_init(&cli_cfg);

  cli_register_simple_command("hello", "Prints Hello World", cmd_hello);

  cli_run();  // Start interactive console
}
```

## Advanced Example with Arguments

```c
static int cmd_echo(cli_context_t *ctx)
{
  const char *msg = ctx->args[0].str_value;
  int repeat = (ctx->args[1].count > 0) ? ctx->args[1].int_value : 1;
  bool uppercase = ctx->args[2].flag_value;

  for (int i = 0; i < repeat; i++) {
    if (uppercase) {
      for (const char *p = msg; *p; p++) {
        printf("%c", (*p >= 'a' && *p <= 'z') ? (*p - 32) : *p);
      }
      printf("\n");
    } else {
      printf("%s\n", msg);
    }
  }
  return 0;
}

static const cli_command_t echo_cmd = {
  .name = "echo",
  .description = "Repeats a message N times",
  .callback = cmd_echo,
  .args = {
    {.short_opt = "m", .long_opt = "msg", .datatype = "<text>",
     .description = "Message to be displayed", .type = CLI_ARG_TYPE_STRING, .required = true},
    {.short_opt = "n", .long_opt = "repeat", .datatype = "<N>",
     .description = "Number of repetitions (default: 1)", .type = CLI_ARG_TYPE_INT, .required = false},
    {.short_opt = "u", .long_opt = "uppercase", .datatype = NULL,
     .description = "Converts to uppercase", .type = CLI_ARG_TYPE_FLAG, .required = false},
  },
  .arg_count = 3,
};

// Register the command
cli_register_command(&echo_cmd);
```

## Included Examples

The project includes four example commands demonstrating different CLI-API features:

1. **`hello`** - Simple command without arguments
2. **`echo`** - Command with string, integer, and flag arguments
3. **`calc`** - Calculator with verbose mode flag
4. **`gpio`** - Complex GPIO configuration with validation and hardware interaction

## How to use example

This example can be used on boards with UART and USB interfaces. The sections below explain how to set up the board and configure the example.

### Using with UART

When UART interface is used, this example can run on any commonly available Espressif development board. UART interface is enabled by default (`CONFIG_ESP_CONSOLE_UART_DEFAULT` option in menuconfig). No extra configuration is required.

### Using with USB_SERIAL_JTAG

*NOTE: We recommend to disable the secondary console output on chips with USB_SERIAL_JTAG since the secondary serial is output-only and would not be very useful when using a console application. This is why the secondary console output is deactivated per default (CONFIG_ESP_CONSOLE_SECONDARY_NONE=y)*

On chips with USB_SERIAL_JTAG peripheral, console example can be used over the USB serial port.

* First, connect the USB cable to the USB_SERIAL_JTAG interface.
* Second, run `idf.py menuconfig` and enable `CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG` option.

For more details about connecting and configuring USB_SERIAL_JTAG (including pin numbers), see the IDF Programming Guide:
* [ESP32-C3 USB_SERIAL_JTAG](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c3/api-guides/usb-serial-jtag-console.html)
* [ESP32-C6 USB_SERIAL_JTAG](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-guides/usb-serial-jtag-console.html)
* [ESP32-S3 USB_SERIAL_JTAG](https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-guides/usb-serial-jtag-console.html)
* [ESP32-H2 USB_SERIAL_JTAG](https://docs.espressif.com/projects/esp-idf/en/stable/esp32h2/api-guides/usb-serial-jtag-console.html)

### Using with USB CDC (USB_OTG peripheral)

USB_OTG peripheral can also provide a USB serial port which works with this example.

* First, connect the USB cable to the USB_OTG peripheral interface.
* Second, run `idf.py menuconfig` and enable `CONFIG_ESP_CONSOLE_USB_CDC` option.

For more details about connecting and configuring USB_OTG (including pin numbers), see the IDF Programming Guide:
* [ESP32-S2 USB_OTG](https://docs.espressif.com/projects/esp-idf/en/stable/esp32s2/api-guides/usb-otg-console.html)
* [ESP32-S3 USB_OTG](https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-guides/usb-otg-console.html)

### Other configuration options

This example has an option to store the command history in Flash. This option is enabled by default.

To disable this, run `idf.py menuconfig` and disable `CONFIG_CONSOLE_STORE_HISTORY` option.

### Configure the project

```
idf.py menuconfig
```

* Enable/Disable storing command history in flash and load the history in a next example run. Linenoise line editing library provides functions to save and load
  command history. If this option is enabled, initializes a FAT filesystem and uses it to store command history.
  * `Example Configuration > Store command history in flash`

* Accept/Ignore empty lines inserted into the console. If an empty line is inserted to the console, the Console can either ignore empty lines (the example would continue), or break on emplty lines (the example would stop after an empty line).
  * `Example Configuration > Ignore empty lines inserted into the console`

### Build and Flash

Build the project and flash it to the board, then run monitor tool to view serial output:

```
idf.py -p PORT flash monitor
```

(Replace PORT with the name of the serial port to use.)

(To exit the serial monitor, type ``Ctrl-]``.)

See the Getting Started Guide for full steps to configure and use ESP-IDF to build projects.

## Example Output

The example starts with a welcome banner and provides several built-in commands:

```
=== ESP32 CLI-API Demo ===
Type 'help' to get the list of commands.
Use UP/DOWN arrows for command history.
Press TAB to auto-complete.

CLI-API Examples:
  hello              - Prints Hello World
  echo -m <msg>      - Repeats message (use -n N, -u)
  calc -a N -b M     - Calculator (use -v for verbose)
  gpio -p N -m MODE  - Configure GPIO (use --pull, -l, -i, -s)
===========================

esp32s3> help
help
  Print the summary of all registered commands if no arguments are given,
  otherwise print summary of given command.

hello
  Prints Hello World

echo
  Repeats a message N times
  -m, --msg=<text>  Message to be displayed
  -n, --repeat=<N>  Number of repetitions (default: 1)
  -u, --uppercase   Converts to uppercase

calc
  Simple calculator (addition, subtraction, multiplication, division)
  -a <num>          First number
  -b <num>          Second number
  -v, --verbose     Shows all operations

gpio
  Configure a GPIO (mode, pull, level)
  -p, --pin=<0-48>        GPIO number
  -m, --mode=<in|out|od>  Mode: in, out, od, inout, inout_od
  --pull=<up|down|none>   Resistor pull: up, down, both, none
  -l, --level=<0|1>       Initial level (for output)
  -i, --info              Show extra GPIO information
  -s, --save              Save configuration to NVS

free
  Get the current size of free heap memory

version
  Get version of chip and SDK

restart
  Software reset of the chip

esp32s3> hello
Hello World! Welcome to ESP32 console!

esp32s3> echo -m "Test" -n 3 -u
TEST
TEST
TEST

esp32s3> calc -a 10 -b 5 -v
Calculating operations with A=10 and B=5
  Addition:        10 + 5 = 15
  Subtraction:     10 - 5 = 5
  Multiplication:  10 * 5 = 50
  Division:        10 / 5 = 2

esp32s3> gpio -p 2 -m out -l 1
+-----------------------------------------+
|       Configuring GPIO 2                |
+-----------------------------------------+
|  Mode:      OUTPUT                      |
|  Pull:      FLOATING                    |
|  Level:     HIGH (1)                    |
|  Status:    OK - Configured             |
+-----------------------------------------+

esp32s3> free
257200

esp32s3> version
ESP32-S3
IDF Version: v5.x
Chip revision: 0

esp32s3> version
ESP32-S3
IDF Version: v5.x
Chip revision: 0
```

## CLI-API Reference

### Core Functions

- **`cli_init(const cli_config_t *config)`** - Initialize the CLI console system
- **`cli_run(void)`** - Start the interactive console loop (blocking)
- **`cli_deinit(void)`** - Clean up and free resources
- **`cli_register_command(const cli_command_t *cmd)`** - Register a command with arguments
- **`cli_register_simple_command(name, description, callback)`** - Register a simple command without arguments
- **`cli_register_commands(commands[], count)`** - Register multiple commands at once

### Configuration Structure

```c
typedef struct {
  const char *prompt;      // Console prompt (e.g., "esp32>")
  const char *banner;      // Welcome message
  bool register_help;      // Auto-register 'help' command
  bool store_history;      // Save command history to flash
} cli_config_t;
```

### Argument Types

- **`CLI_ARG_TYPE_INT`** - Integer argument (e.g., `--timeout 5000`)
- **`CLI_ARG_TYPE_STRING`** - String argument (e.g., `--ssid "MyNetwork"`)
- **`CLI_ARG_TYPE_FLAG`** - Boolean flag (e.g., `-v` or `--verbose`)

### Command Context

Commands receive a `cli_context_t` pointer containing:
- `argc` / `argv` - Original arguments
- `args[]` - Parsed argument values with `.int_value`, `.str_value`, `.flag_value`, and `.count`
- `arg_count` - Number of defined arguments

## Project Structure

```
cli-api/
├── components/
│   ├── cli-api/              # Main CLI-API library
│   │   ├── include/cli-api.h # Public API header
│   │   └── cli-api.c         # Implementation
│   ├── cmd_system/           # System commands (free, heap, version, restart)
│   ├── cmd_wifi/             # WiFi commands (scan, connect, disconnect)
│   └── cmd_nvs/              # NVS commands (get, set, erase)
└── main/
    └── main.c                # Example application with CLI-API demos
```

## Troubleshooting

### Line Endings

The line endings in the Console Example are configured to match particular serial monitors. Therefore, if the following log output appears, consider using a different serial monitor (e.g. Putty for Windows) or modify the example's [UART configuration](#Configuring-UART).

```
This is an example of ESP-IDF console component.
Type 'help' to get the list of commands.
Use UP/DOWN arrows to navigate through command history.
Press TAB when typing command name to auto-complete.
Your terminal application does not support escape sequences.
Line editing and history features are disabled.
On Windows, try using Windows Terminal or Putty instead.
esp32>
```

### Escape Sequences on Windows 10

When using the default command line or PowerShell on Windows 10, you may see a message indicating that the console does not support escape sequences, as shown in the above output. To avoid such issues, it is recommended to run the serial monitor under [Windows Terminal](https://en.wikipedia.org/wiki/Windows_Terminal), which supports all required escape sequences for the app, unlike the default terminal. The main escape sequence of concern is the Device Status Report (`0x1b[5n`), which is used to check terminal capabilities. Any response to this sequence indicates support. This should not be an issue on Windows 11, where Windows Terminal is the default.

### No USB port appears

On Windows 10, macOS, Linux, USB CDC devices do not require additional drivers to be installed.

If the USB serial port doesn't appear in the system after flashing the example, check the following:

* Check that the USB device is detected by the OS.
  VID/PID pair for ESP32-S2 is 303a:0002.

  - On Windows, check the Device Manager
  - On macOS, check USB section in the System Information utility
  - On Linux, check `lsusb` output

* If the device is not detected, check the USB cable connection (D+, D-, and ground should be connected)

## Implementation Details

### CLI-API Initialization

The `cli_init()` function handles:
- NVS initialization for persistent storage
- FATFS setup for command history
- Console peripheral configuration (UART/USB)
- Linenoise library setup with line editing, completion, and history

### Command Registration

CLI-API provides two registration methods:

1. **Simple Commands** (no arguments):
   ```c
   cli_register_simple_command("hello", "Prints Hello World", cmd_hello);
   ```

2. **Complex Commands** (with arguments):
   ```c
   cli_register_command(&echo_cmd);
   ```

The library automatically:
- Allocates argtable3 structures
- Configures argument parsing
- Registers with esp_console
- Handles cleanup on errors

### Argument Parsing

The CLI-API wrapper automatically:
- Parses command-line arguments using argtable3
- Validates required vs optional arguments
- Converts strings to appropriate types (int, string, bool)
- Provides parsed values in a clean `cli_context_t` structure
- Displays helpful error messages on invalid input

### Command History

When `store_history = true`:
- A FAT filesystem is mounted on the "storage" partition
- Command history is saved to `/data/history.txt`
- History persists across reboots
- Accessible via UP/DOWN arrow keys in the console

## Key Changes from ESP-IDF Console Example

This project improves upon the standard ESP-IDF console example:

1. **Simplified API**: No need to manually create argtable3 structures
2. **Clean Headers**: Removed `extern "C"` wrappers (pure C implementation)
3. **English Codebase**: All comments and messages in English (no Portuguese)
4. **Structured Arguments**: Declarative command definitions with type safety
5. **Reusable Component**: Easy to integrate into other ESP32 projects

## License

This project is in the Public Domain (CC0 licensed). See individual component licenses for third-party code.

## References

- [ESP-IDF Console Component Documentation](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/console.html)
- [Argtable3 Library](http://www.argtable.org/)
- [Linenoise Line Editing Library](https://github.com/antirez/linenoise)
- [ESP-IDF Programming Guide](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/)
