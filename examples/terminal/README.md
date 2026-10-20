# CLI-API Terminal Example

This example demonstrates the **CLI-API** terminal features: line editing (TAB completion, argument hints, UP/DOWN history, Backspace), the `Terminal mode` menuconfig option, and the `history` / `sync` commands.

## Commands

| Command              | Description                                                  |
|----------------------|--------------------------------------------------------------|
| `term`               | Shows the configured terminal mode and the one actually in use |
| `counter-add -n <N>` | Adds `N` to a counter (shows an argument hint while typing)  |
| `counter-show`       | Prints the counter                                           |
| `counter-reset`      | Resets the counter to 0                                      |
| `history`            | Lists the in-memory command history, numbered from 1         |
| `history -c`         | Clears the history (UP/DOWN recall too)                      |
| `sync`               | Saves the history to `/data/history.txt` on the "storage" partition |

`history` and `sync` are registered by the component itself because the example sets `cli_config_t.history_sync = true`.

## How to Use

### Build and Flash

```bash
idf.py -p PORT flash monitor
```

(Replace `PORT` with the serial port name, e.g., `/dev/ttyUSB0` or `COM3`.)

### Try It

1. Run `term`. It should report `Mode in use: line editing`. If it reports `dumb`, see [Terminal Mode](#terminal-mode) below.
2. Type `cou` and press **TAB**. The line completes to `counter-add`. Press **TAB** again to cycle through `counter-reset` and `counter-show`.
3. Type `counter-add` followed by a space. The hint `-n <value>` appears after the cursor.
4. Run `counter-add -n 5`, then `counter-show`. Press **UP** and **DOWN** to browse them.
5. Type a few characters and press **Backspace**. Each press erases a character and moves the cursor back.
6. Run `history` to list everything typed so far, then `history -c` to clear it. Press **UP**: nothing is recalled.
7. Run `sync` to save the history to flash, then reset the board. `history` and **UP** still show the saved commands.

### Example Output

```bash
=== CLI-API Terminal Example ===
Try the line editing features:
  TAB        complete a command (type 'cou', then TAB again to cycle)
  UP/DOWN    browse previous commands
  Backspace  erase the last character

Commands:
  term               show the configured and in-use terminal mode
  counter-add -n N   add N to the counter (watch the hint while typing)
  counter-show       print the counter
  counter-reset      reset the counter
  history [-c]       list (or clear) the command history
  sync               save the history to flash
================================

term> term
Configured mode: Auto-detect
Mode in use:     line editing

term> counter-add -n 5
Counter: 5

term> counter-show
Counter: 5

term> history
   1  term
   2  counter-add -n 5
   3  counter-show
   4  history

term> history -c
History cleared
```

## Terminal Mode

Line editing needs a terminal that understands VT100 escape sequences. The component picks the mode at boot according to `idf.py menuconfig` > `CLI API Configuration` > `Terminal mode`:

| Option               | Behavior                                                                 | When to pick it |
|----------------------|--------------------------------------------------------------------------|-----------------|
| Auto-detect (default) | Sends a Device Status Request (`ESC[5n`) at boot; line editing if the terminal answers, dumb mode otherwise. USB Serial JTAG always gets line editing. | Most setups (`idf.py monitor`, PuTTY, Windows Terminal, VS Code). |
| Force line editing   | Skips the probe, always line editing.                                    | Your terminal supports VT100 but `term` reports `dumb` under Auto. |
| Force dumb mode      | Skips the probe, always dumb mode (plain line input, no TAB/arrows).      | Web Serial pages and other hosts that print escape sequences as text. |

This example's [sdkconfig.defaults](sdkconfig.defaults) selects Auto and lists the other two options as comments.

## Code Highlight

Turning on `history_sync` registers both `history` and `sync`:

```c
cli_config_t cli_cfg = {
  .prompt = "term> ",
  .register_help = true,
  .history_sync = true, /* registers 'history' and 'sync' */
};
ESP_ERROR_CHECK(cli_init(&cli_cfg));
```

A command with arguments and `.hint = NULL` gets its hint generated from those arguments:

```c
static const cli_command_t counter_add_cmd = {
  .name = "counter-add",
  .description = "Adds a value to the counter",
  .hint = NULL, /* NULL: esp_console builds the hint from the arguments */
  .callback = cmd_counter_add,
  .args = {{.short_opt = "n", .long_opt = "value", .datatype = "<value>",
            .description = "Value to add (may be negative)", .type = CLI_ARG_TYPE_INT, .required = true}},
  .arg_count = 1,
};
```

## Project Structure

```text
examples/terminal/
├── CMakeLists.txt
├── partitions_example.csv
├── sdkconfig.defaults
├── README.md
└── main/
    ├── CMakeLists.txt
    ├── idf_component.yml
    └── main.c
```
