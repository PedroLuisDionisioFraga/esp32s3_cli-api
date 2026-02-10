# CLI-API Advanced Example

This example demonstrates the full capabilities of the **CLI-API** component, including typed argument commands, GPIO hardware interaction, and integration with **cmd_system**, **cmd_wifi**, and **cmd_nvs** components.

## Commands

### CLI-API Example Commands

| Command | Description | Arguments |
|---------|-------------|-----------|
| `echo`  | Repeats a message N times | `-m <text>` (required), `-n <N>` (optional), `-u` flag |
| `calc`  | Simple calculator | `-a <num>` (required), `-b <num>` (required), `-v` flag |
| `gpio`  | Configure a GPIO pin | `-p <pin>`, `-m <mode>`, `--pull`, `-l`, `-i`, `-s` |

### System Commands (cmd_system)

| Command   | Description |
|-----------|-------------|
| `free`    | Get free heap memory size |
| `heap`    | Get minimum free heap size |
| `version` | Get chip and SDK version |
| `restart` | Software reset |
| `tasks`   | List running FreeRTOS tasks |
| `light_sleep` / `deep_sleep` | Enter sleep mode (if supported) |

### WiFi Commands (cmd_wifi)

| Command | Description |
|---------|-------------|
| `join`  | Connect to a WiFi network |
| `scan`  | Scan for available networks |

### NVS Commands (cmd_nvs)

| Command    | Description |
|------------|-------------|
| `nvs_set`  | Set a key-value pair in NVS |
| `nvs_get`  | Get a value from NVS |
| `nvs_erase`| Erase a key from NVS |

## How to Use

### Build and Flash

```bash
idf.py -p PORT flash monitor
```

(Replace `PORT` with the serial port name, e.g., `/dev/ttyUSB0` or `COM3`.)

### Example Output

```
=== ESP32 CLI-API Advanced Example ===
Type 'help' to get the list of commands.
Use UP/DOWN arrows for command history.
Press TAB to auto-complete.

CLI-API Examples:
  echo -m <msg>      - Repeats message (use -n N, -u)
  calc -a N -b M     - Calculator (use -v for verbose)
  gpio -p N -m MODE  - Configure GPIO (use --pull, -l, -i, -s)

System / WiFi / NVS commands also available.
=======================================

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
IDF Version: v5.5.2
Chip revision: 0
```

## Code Highlights

### Argument-based command registration

```c
static const cli_command_t echo_cmd = {
  .name = "echo",
  .description = "Repeats a message N times",
  .callback = cmd_echo,
  .args = {
    { .short_opt = "m", .long_opt = "msg",       .type = CLI_ARG_TYPE_STRING, .required = true  },
    { .short_opt = "n", .long_opt = "repeat",     .type = CLI_ARG_TYPE_INT,    .required = false },
    { .short_opt = "u", .long_opt = "uppercase",  .type = CLI_ARG_TYPE_FLAG,   .required = false },
  },
  .arg_count = 3,
};
cli_register_command(&echo_cmd);
```

### Accessing parsed arguments in callbacks

```c
static int cmd_echo(cli_context_t *ctx)
{
  const char *msg  = ctx->args[0].str_value;                             // required string
  int repeat       = (ctx->args[1].count > 0) ? ctx->args[1].int_value : 1; // optional int
  bool uppercase   = ctx->args[2].flag_value;                            // optional flag
  // ...
}
```

### Integration with system/wifi/nvs components

```c
register_system_common();          // free, heap, version, restart, tasks
register_system_light_sleep();     // light_sleep (if supported)
register_system_deep_sleep();      // deep_sleep (if supported)
register_wifi();                   // join, scan
register_nvs();                    // nvs_set, nvs_get, nvs_erase
```

## Project Structure

```
examples/advanced/
├── CMakeLists.txt
├── partitions_example.csv
├── sdkconfig.defaults
├── README.md
└── main/
    ├── CMakeLists.txt
    ├── idf_component.yml
    └── advanced_example_main.c
```
