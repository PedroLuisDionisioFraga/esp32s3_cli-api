# CLI-API - Simplified Console API for ESP32

A simplified, header-only like API for creating command-line interfaces (CLI) on ESP32 devices. This project wraps the complexity of ESP-IDF's console component and argtable3 library into an easy-to-use interface.

## Requirements

- ESP-IDF >= 6.0 (tested on v6.0.2)

## Installation

Add the component to your project's `main/idf_component.yml`:

```yaml
dependencies:
  pedroluisdionisiofraga/cli-api: "*"
  idf: ">=6.0.0"
```

`"*"` always pulls the latest version published to the ESP Component Registry.
Pin an explicit version (for example `"1.0.4"`) only if you need reproducible builds.

Equivalently, from the project root:

```bash
idf.py add-dependency "pedroluisdionisiofraga/cli-api"
```

## Features

- **Simple Command Registration**: Define commands using declarative structs instead of complex argtable3 code
- **Multiple Argument Types**: Support for integers, strings, and boolean flags
- **Automatic Help Generation**: Built-in help system for all registered commands
- **Line Editing**: TAB completion, argument hints and UP/DOWN history, with the terminal mode auto-detected (or forced from menuconfig)
- **Command History**: kept in RAM; the `history` command lists or clears it, the `sync` command persists it to flash on demand and it is restored on the next boot
- **Multiple Interface Support**: UART, USB_SERIAL_JTAG, and USB_CDC
- **Example Commands**: Includes WiFi, NVS, and system management commands
- **Clean C API**: No extern "C" wrappers needed, pure C implementation

## How to use example

This example can be used on boards with UART and USB interfaces. The sections below explain how to set up the board and configure the example.

### Using with UART

When UART interface is used, this example can run on any commonly available Espressif development board. UART interface is enabled by default (`CONFIG_ESP_CONSOLE_UART_DEFAULT` option in menuconfig). No extra configuration is required.

### Using with USB_SERIAL_JTAG

**NOTE**: We recommend to disable the secondary console output on chips with USB_SERIAL_JTAG since the secondary serial is output-only and would not be very useful when using a console application. This is why the secondary console output is deactivated per default (CONFIG_ESP_CONSOLE_SECONDARY_NONE=y)

On chips with USB_SERIAL_JTAG peripheral, console example can be used over the USB serial port.

- First, connect the USB cable to the USB_SERIAL_JTAG interface.
- Second, run `idf.py menuconfig` and enable `CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG` option.

For more details about connecting and configuring USB_SERIAL_JTAG (including pin numbers), see the IDF Programming Guide:

- [ESP32-C3 USB_SERIAL_JTAG](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c3/api-guides/usb-serial-jtag-console.html)
- [ESP32-C6 USB_SERIAL_JTAG](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-guides/usb-serial-jtag-console.html)
- [ESP32-S3 USB_SERIAL_JTAG](https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-guides/usb-serial-jtag-console.html)
- [ESP32-H2 USB_SERIAL_JTAG](https://docs.espressif.com/projects/esp-idf/en/stable/esp32h2/api-guides/usb-serial-jtag-console.html)

### Using with USB CDC (USB_OTG peripheral)

USB_OTG peripheral can also provide a USB serial port which works with this example.

- First, connect the USB cable to the USB_OTG peripheral interface.
- Second, run `idf.py menuconfig` and enable `CONFIG_ESP_CONSOLE_USB_CDC` option.

For more details about connecting and configuring USB_OTG (including pin numbers), see the IDF Programming Guide:

- [ESP32-S2 USB_OTG](https://docs.espressif.com/projects/esp-idf/en/stable/esp32s2/api-guides/usb-otg-console.html)
- [ESP32-S3 USB_OTG](https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-guides/usb-otg-console.html)

### Other configuration options

Nothing is written to flash unless you explicitly run the `sync` command, and with `history_sync` the saved history is restored on the next boot (see [Command History](#command-history) below). Registering the `history` and `sync` commands is a compile-time choice (`cli_config_t.history_sync`), not a menuconfig option.

Line editing (TAB completion, hints, UP/DOWN history) requires a terminal that understands VT100 escape sequences. `CLI API Configuration > Terminal mode` selects how the component decides:

- **Auto-detect** (default): probes the terminal at boot (`ESC[5n`) and falls back to dumb mode only if it doesn't answer within 500 ms. If the terminal sends an escape sequence later (an arrow key, or the probe reply arriving late because the host app had just connected), line editing is enabled from the next prompt. USB_SERIAL_JTAG always gets line editing, since the host is usually not attached yet at boot.
- **Force line editing**: skips the probe. Use it if your terminal supports VT100 but Auto reports it doesn't.
- **Force dumb mode**: skips the probe. Use it for Web Serial and other hosts that print escape sequences as text.

The [terminal example](examples/terminal) shows all of this in action.

Whether an empty line (e.g. EOF/Ctrl+C on a dumb terminal) breaks the console loop or is silently ignored is controlled by this component's own `CLI API Configuration > Ignore empty lines returned by the console` menuconfig option, enabled (ignore) by default.

### Configure the project

```powershell
idf.py menuconfig
```

### Build and Flash

Build the project and flash it to the board, then run monitor tool to view serial output:

```powershell
idf.py -p PORT flash monitor
```

(Replace PORT with the name of the serial port to use.)

(To exit the serial monitor, type ``Ctrl-]``.)

See the Getting Started Guide for full steps to configure and use ESP-IDF to build projects.

## Data Structures

The diagram below shows all the structs in the project and how they relate to each other:

```mermaid
classDiagram
    direction TB

    class cli_arg_type_t {
        <<enum>>
        CLI_ARG_TYPE_INT
        CLI_ARG_TYPE_STRING
        CLI_ARG_TYPE_FLAG
    }

    class cli_arg_t {
        <<public - descriptor>>
        const char* short_opt
        const char* long_opt
        const char* datatype
        const char* description
        cli_arg_type_t type
        bool required
    }

    class cli_arg_value_t {
        <<public - parsed>>
        int int_value
        const char* str_value
        bool flag_value
        int count
    }

    class cli_context_t {
        <<public - callback context>>
        int argc
        char** argv
        cli_arg_value_t args[CLI_MAX_ARGS]
        uint8_t arg_count
    }

    class cli_command_t {
        <<public - command descriptor>>
        const char* name
        const char* description
        const char* hint
        cli_callback_t callback
        cli_arg_t args[CLI_MAX_ARGS]
        uint8_t arg_count
    }

    class cli_config_t {
        <<public - init config>>
        const char* prompt
        const char* banner
        bool register_help
        bool history_sync
    }

    class cli_registered_cmd_t {
        <<internal>>
        const cli_command_t* cmd_def
        void* argtable[CLI_MAX_ARGS+1]
        uint8_t arg_count
    }

    class cli_state_t {
        <<internal - singleton>>
        char prompt[CLI_PROMPT_MAX_LEN]
        bool initialized
        wl_handle_t wl_handle
        cli_registered_cmd_t cmds[CLI_MAX_COMMANDS]
        uint8_t cmd_count
    }

    cli_arg_t --> cli_arg_type_t : type field
    cli_command_t *-- cli_arg_t : args[CLI_MAX_ARGS]
    cli_context_t *-- cli_arg_value_t : args[CLI_MAX_ARGS]
    cli_registered_cmd_t --> cli_command_t : cmd_def pointer
    cli_state_t *-- cli_registered_cmd_t : cmds[CLI_MAX_COMMANDS]
    cli_state_t ..> cli_config_t : configured by cli_init()
    cli_arg_t ..> cli_arg_value_t : wrapper converts
```

**Flow summary:**

- **Registration:** You define a `cli_command_t` (which contains `cli_arg_t` descriptors). When registered, a `cli_registered_cmd_t` is created internally with argtable3 structs and stored in `cli_state_t`.
- **Execution:** When the user types a command, the wrapper parses arguments via argtable3, converts them into `cli_arg_value_t` values, packs everything into a `cli_context_t`, and calls your callback.
- **Initialization:** `cli_config_t` is passed to `cli_init()` which copies its values into the `cli_state_t` singleton.

### Visual Flow

The example below shows how a user-defined command (`echo`) is stored internally and linked back through `cmd_def`:

```text
User defines:                       API stores internally:

cli_command_t echo_cmd = {          cli_registered_cmd_t {
  .name = "echo",                     .cmd_def ──────────► &echo_cmd
  .callback = cmd_echo,               .argtable[0] = arg_str1(...)  ← generated
  .args[0] = { "-m", ... },           .argtable[1] = arg_int0(...)  ← generated
  .args[1] = { "-n", ... },           .argtable[2] = arg_end(...)   ← generated
  .arg_count = 2,                     .arg_count = 2
};                                  }
```

At **execution time**, when the user types `echo -m "hello" -n 3`:

```text
1. esp_console calls cli_command_wrapper(argc, argv)
                          │
2. Finds matching         │    cli_registered_cmd_t.cmd_def->name == "echo"
   registered command     │
                          ▼
3. arg_parse()       argtable[0] (arg_str) ← "hello"
   fills argtable    argtable[1] (arg_int) ← 3
                          │
4. Wrapper reads          │    cmd_def->args[i].type tells how to extract
   each arg type          │
                          ▼
5. Builds context    cli_context_t {
                       .args[0] = { .str_value = "hello", .count = 1 }
                       .args[1] = { .int_value = 3,       .count = 1 }
                     }
                          │
6. Calls callback         ▼
                     cmd_def->callback(&ctx)  →  cmd_echo(ctx)
```

## CLI-API Reference

### Core Functions

- **`cli_init(const cli_config_t *config)`** - Initialize the CLI console system
- **`cli_run(void)`** - Start the interactive console loop (blocking)
- **`cli_deinit(void)`** - Clean up and free resources
- **`cli_register_command(const cli_command_t *cmd)`** - Register a command with arguments
- **`cli_register_simple_command(name, description, callback)`** - Register a simple command without arguments
- **`cli_register_commands(commands[], count)`** - Register multiple commands at once

## Troubleshooting

### Line Endings

The line endings in the Console Example are configured to match particular serial monitors. Therefore, if the following log output appears, consider using a different serial monitor (e.g. Putty for Windows) or modify the example's [UART configuration](#configure-the-project).

```text
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

If your terminal does support escape sequences but still gets the message (for example because the monitor attached after the probe ran at boot), Auto-detect switches to line editing as soon as the terminal sends an escape sequence, so the first prompt may be plain. To avoid that, select `CLI API Configuration > Terminal mode > Force line editing` in menuconfig to skip the probe.

### No USB port appears

On Windows 10, macOS, Linux, USB CDC devices do not require additional drivers to be installed.

If the USB serial port doesn't appear in the system after flashing the example, check the following:

- Check that the USB device is detected by the OS.
  VID/PID pair for ESP32-S2 is 303a:0002.

  - On Windows, check the Device Manager
  - On macOS, check USB section in the System Information utility
  - On Linux, check `lsusb` output

- If the device is not detected, check the USB cable connection (D+, D-, and ground should be connected)

## Implementation Details

### CLI-API Initialization

The `cli_init()` function handles:

- NVS initialization
- Console peripheral configuration (UART/USB)
- Linenoise library setup with line editing, completion, and (RAM-only) history, in the mode selected by `CLI API Configuration > Terminal mode`
- Optionally registers the `history` and `sync` commands (see [Command History](#command-history))

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

Command history lives in RAM (via linenoise's own history buffer) and is accessible with the UP/DOWN arrow keys. Nothing is ever written to flash automatically; without `history_sync` it is cleared on every reset.

When `history_sync = true`, `cli_init()` also registers two commands and restores the saved history:

- `history` lists the in-memory history, numbered from 1 (oldest first), and `history -c` clears it, UP/DOWN recall included.
- `sync` saves the current in-memory history to `/data/history.txt` on the "storage" partition.
- At boot, `cli_init()` mounts the "storage" partition and loads `/data/history.txt` back, so commands saved with `sync` are available to both UP/DOWN and `history` after a reset. A missing file (nothing synced yet) is ignored.
- If no such partition exists, `cli_init()` logs a warning and history simply starts empty, and `sync` fails with a clear error telling you to add the partition.

## References

- [ESP-IDF Console Component Documentation](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/console.html)
- [Argtable3 Library](http://www.argtable.org/)
- [Linenoise Line Editing Library](https://github.com/antirez/linenoise)
- [ESP-IDF Programming Guide](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/)
