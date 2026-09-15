# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Changed

- **Breaking:** command history is now RAM-only by default and cleared on every reset. `cli_config_t.store_history` was renamed to `history_sync`; when `true`, `cli_init()` registers a new `sync` command that persists the in-memory history to `/data/history.txt` on demand, instead of every command line being auto-saved to flash. This removes an unconditional per-command flash write (unnecessary wear) and, since the "storage" partition is now mounted lazily on the first `sync` instead of at boot, a consumer that never runs `sync` has no dependency on that partition existing at all — one that previously failed silently with just a boot-time log line now fails loudly, in `sync`'s own output, exactly when it matters.
- Removed the now-dead `CONSOLE_STORE_HISTORY` example Kconfig option (never actually read by either example's `main.c`) and the `CONSOLE_IGNORE_EMPTY_LINES` example option superseded by the component's own `CLI_API_IGNORE_EMPTY_LINES` (see 1.0.7 below).

### Migration from 1.0.7

```c
/* Before */
cli_config_t cli_cfg = { .store_history = true, /* ... */ };

/* After */
cli_config_t cli_cfg = { .history_sync = true, /* ... */ };
```

History is available via UP/DOWN arrows exactly as before; run `sync` when you want it saved to flash.

## [1.0.7] - 2026-09-14

### Fixed

- Command lines with more than eight tokens (e.g. several `--option value` pairs) no longer lose trailing arguments: `esp_console` now uses `CLI_MAX_CMDLINE_ARGS` (32) instead of reusing `CLI_MAX_ARGS` (8).

## [1.0.4] - 2026-07-11

### Added

- ESP-IDF v6.0 support: component and both examples build against ESP-IDF v6.0.2.
- `build.yml` CI workflow building the `basic` and `advanced` examples on ESP-IDF `release-v6.0` and `latest`, across esp32, esp32s3, esp32c3, esp32c6 and esp32h2.
- `format.yml` CI workflow enforcing clang-format on pull requests.
- `CHANGELOG.md`.

### Changed

- **Breaking:** minimum supported ESP-IDF raised to v6.0.0. ESP-IDF v5.x is no longer supported.
- Example manifests now use `override_path` so local/CI builds test the repository code instead of the published Registry version.
- Devcontainer image pinned to `espressif/idf:v6.0.2`.
- Manifest `name` normalized to `cli-api` (lowercase, registry-safe).

### Fixed

- Missing FreeRTOS includes in both examples' `main.c` (build error on ESP-IDF v6).
- `advanced` example used `esp_sleep_get_wakeup_cause()`, deprecated in ESP-IDF v6; it now selects the `esp_sleep_get_wakeup_causes()` bitmap API on v6 and above.

## [1.0.3] - 2026-07-09

### Added

- Upload to ESP Component Registry skipped with a warning when the version is already published.

## [1.0.2] and earlier

- Initial releases published to the ESP Component Registry.
