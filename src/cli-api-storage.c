/**
 * @file cli-api-storage.c
 * @author Pedro Luis Dionísio Fraga (pedrodfraga@hotmail.com)
 *
 * @brief NVS/FATFS setup and the optional 'sync' command for history persistence.
 *
 * This is the only file that touches `s_cli.wl_handle`. The FATFS partition is
 * mounted lazily, the first time 'sync' runs — never at boot — so a consumer
 * that never calls 'sync' has no dependency on a "storage" partition existing
 * at all, and a missing partition is reported right where it matters (in the
 * command's own output) instead of as an easy-to-miss boot-time log line.
 *
 * @version 0.1
 * @date 2026-02-05
 *
 * @copyright Copyright (c) 2026
 *
 */

#include <esp_log.h>
#include <linenoise/linenoise.h>
#include <nvs.h>
#include <nvs_flash.h>
#include <stdio.h>

#include "cli-api-private.h"

static const char *TAG = "cli-api";

esp_err_t cli_init_nvs(void)
{
  esp_err_t err = nvs_flash_init();
  if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
  {
    ESP_LOGW(TAG, "NVS partition truncated, erasing...");
    ESP_ERROR_CHECK(nvs_flash_erase());
    err = nvs_flash_init();
  }
  if (err == ESP_OK)
    ESP_LOGI(TAG, "NVS initialized");

  return err;
}

esp_err_t cli_init_filesystem(void)
{
  const esp_vfs_fat_mount_config_t mount_config = {.max_files = 4,
                                                   .format_if_mount_failed = true,
                                                   .allocation_unit_size = CONFIG_WL_SECTOR_SIZE};

  esp_err_t err = esp_vfs_fat_spiflash_mount_rw_wl(CLI_MOUNT_PATH, "storage", &mount_config, &s_cli.wl_handle);
  if (err != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to mount FATFS (%s)", esp_err_to_name(err));
    return err;
  }

  ESP_LOGI(TAG, "FATFS mounted at %s", CLI_MOUNT_PATH);
  return ESP_OK;
}

void cli_deinit_filesystem(void)
{
  if (s_cli.wl_handle != WL_INVALID_HANDLE)
  {
    esp_vfs_fat_spiflash_unmount_rw_wl(CLI_MOUNT_PATH, s_cli.wl_handle);
    s_cli.wl_handle = WL_INVALID_HANDLE;
    ESP_LOGI(TAG, "FATFS unmounted");
  }
}

/**
 * @brief "sync" command: mounts the storage partition on first use (if not
 * already mounted) and writes the current in-memory command history to it.
 *
 * Nothing else in this component ever writes history to flash — this is the
 * only path that does, and only when the user explicitly asks for it.
 */
static int cmd_sync(int argc, char **argv)
{
  if (s_cli.wl_handle == WL_INVALID_HANDLE)
  {
    esp_err_t err = cli_init_filesystem();
    if (err != ESP_OK)
    {
      printf("Failed to mount the \"storage\" partition (%s).\n"
             "Add a data/fat partition named \"storage\" to your partition table to enable 'sync'.\n",
             esp_err_to_name(err));
      return 1;
    }
  }

  if (linenoiseHistorySave(CLI_HISTORY_PATH) != 0)
  {
    printf("Failed to save history to %s\n", CLI_HISTORY_PATH);
    return 1;
  }

  printf("History saved to %s\n", CLI_HISTORY_PATH);
  return 0;
}

void cli_register_history_sync_command(void)
{
  cli_register_simple_command("sync", "Persist the in-memory command history to flash", cmd_sync);
}
