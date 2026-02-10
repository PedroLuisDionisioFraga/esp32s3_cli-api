# CLI-API Basic Example

This example demonstrates the simplest way to use the **CLI-API** component: registering commands with `cli_register_simple_command()`.

Simple commands receive standard `(int argc, char **argv)` parameters — no argument parsing needed.

## Commands

| Command  | Description                          |
|----------|--------------------------------------|
| `hello`  | Prints "Hello World!"                |
| `status` | Shows free heap, min heap, IDF version |
| `about`  | Prints project information           |

## How to Use

### Build and Flash

```bash
idf.py -p PORT flash monitor
```

(Replace `PORT` with the serial port name, e.g., `/dev/ttyUSB0` or `COM3`.)

### Example Output

```
=== CLI-API Basic Example ===
Type 'help' to get the list of commands.
Use UP/DOWN arrows for command history.
Press TAB to auto-complete.
=============================

basic> hello
Hello World! Welcome to ESP32 console!

basic> status
+--------------------------+
|     System Status        |
+--------------------------+
|  Free heap:  257200 B    |
|  Min heap:   245680 B    |
|  IDF ver:    v5.5.2      |
+--------------------------+

basic> about
CLI-API Basic Example
  A simplified API for ESP-IDF console commands.
  See: https://github.com/pedrodfraga/cli-api
```

## Code Highlight

The key API usage is just three lines per command:

```c
cli_register_simple_command("hello",  "Prints Hello World",   cmd_hello);
cli_register_simple_command("status", "Shows system status",  cmd_status);
cli_register_simple_command("about",  "Prints project info",  cmd_about);
```

Each callback has the signature:

```c
static int cmd_hello(int argc, char **argv)
{
    printf("Hello World!\n");
    return 0;  // 0 = success
}
```

## Project Structure

```
examples/basic/
├── CMakeLists.txt
├── partitions_example.csv
├── sdkconfig.defaults
├── README.md
└── main/
    ├── CMakeLists.txt
    ├── idf_component.yml
    └── basic_example_main.c
```
