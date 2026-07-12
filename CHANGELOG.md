# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

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
