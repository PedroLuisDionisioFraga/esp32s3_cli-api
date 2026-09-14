/**
 * @file cli-api-storage.c
 * @author Pedro Luis Dionísio Fraga (pedrodfraga@hotmail.com)
 *
 * @brief NVS and FATFS setup for command history persistence.
 *
 * This is the only file that touches `s_cli.wl_handle`.
 *
 * @version 0.1
 * @date 2026-02-05
 *
 * @copyright Copyright (c) 2026
 *
 */

#include "cli-api-private.h"

#include <esp_log.h>
#include <nvs.h>
#include <nvs_flash.h>

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
    ESP_LOGE(TAG, "Failed to mount FATFS (%s). History disabled.", esp_err_to_name(err));
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
