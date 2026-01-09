#include "sd_card_manager.h"
#include "driver/sdmmc_host.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "sd_pwr_ctrl.h"
#include "sd_pwr_ctrl_by_on_chip_ldo.h"
#include "sdmmc_cmd.h"

static const char *TAG = "SD_CARD";
static sdmmc_card_t *s_card = NULL;
static bool s_mounted = false;
static sd_pwr_ctrl_handle_t s_pwr_ctrl_handle = NULL;

static esp_err_t host_init_dummy(void) {
  // Host already initialized by another component (like WiFi)
  return ESP_OK;
}

esp_err_t sd_card_init(void) {
  if (s_mounted) {
    ESP_LOGW(TAG, "SD Card already mounted");
    return ESP_OK;
  }

  ESP_LOGI(TAG, "Initializing SD card");

  esp_vfs_fat_sdmmc_mount_config_t mount_config = {
      .format_if_mount_failed = true,
      .max_files = 20,
      .allocation_unit_size = 16 * 1024};

  ESP_LOGI(TAG, "Using SDMMC peripheral");
  sdmmc_host_t host = SDMMC_HOST_DEFAULT();
  host.slot = SD_SLOT_NUM;
  host.max_freq_khz = SD_FREQ_KHZ;

#ifdef CONFIG_IDF_TARGET_ESP32P4
  host.flags |= SDMMC_HOST_FLAG_ALLOC_ALIGNED_BUF;
#endif

  // Check if host is already initialized (e.g. by WiFi SDIO)
  esp_err_t host_ret = sdmmc_host_init();
  if (host_ret == ESP_ERR_INVALID_STATE) {
    ESP_LOGI(TAG, "SDMMC host already initialized (shared)");
    host.init = host_init_dummy;
  } else if (host_ret != ESP_OK) {
    ESP_LOGE(TAG, "Failed to initialize SDMMC host (%s)",
             esp_err_to_name(host_ret));
    return host_ret;
  } else { // host_ret == ESP_OK
    ESP_LOGI(TAG, "SDMMC host initialized successfully");
  }

#ifdef CONFIG_IDF_TARGET_ESP32P4
  // Initialize power control using internal LDO (Channel 4 for Waveshare board)
  sd_pwr_ctrl_ldo_config_t ldo_config = {
      .ldo_chan_id = 4,
  };
  esp_err_t pwr_ret =
      sd_pwr_ctrl_new_on_chip_ldo(&ldo_config, &s_pwr_ctrl_handle);
  if (pwr_ret != ESP_OK) {
    ESP_LOGE(TAG, "Failed to create a new on-chip LDO power control driver");
    return pwr_ret;
  }
  host.pwr_ctrl_handle = s_pwr_ctrl_handle;
#endif

  sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
  slot_config.width = 4;

  // On ESP32-P4, Slot 0 uses specific dedicated IOMUX pins.
  // We leave them as default (NC) to ensure the driver use the dedicated
  // hardware.
  slot_config.clk = GPIO_NUM_NC;
  slot_config.cmd = GPIO_NUM_NC;
  slot_config.d0 = GPIO_NUM_NC;
  slot_config.d1 = GPIO_NUM_NC;
  slot_config.d2 = GPIO_NUM_NC;
  slot_config.d3 = GPIO_NUM_NC;
  slot_config.cd = GPIO_NUM_NC;
  slot_config.wp = GPIO_NUM_NC;
  slot_config.flags |= SDMMC_SLOT_FLAG_INTERNAL_PULLUP;

  ESP_LOGI(TAG, "Mounting filesystem");
  esp_err_t ret = esp_vfs_fat_sdmmc_mount(SD_MOUNT_POINT, &host, &slot_config,
                                          &mount_config, &s_card);

  if (ret != ESP_OK) {
    if (ret == ESP_FAIL) {
      ESP_LOGE(TAG, "Failed to mount filesystem. "
                    "If you want the card to be formatted, set the "
                    "CONFIG_FATFS_FORMAT_IF_MOUNT_FAILED menuconfig option.");
    } else {
      ESP_LOGE(TAG,
               "Failed to initialize the card (%s). "
               "Make sure SD card lines have pull-up resistors in place.",
               esp_err_to_name(ret));
    }
    return ret;
  }

  ESP_LOGI(TAG, "Filesystem mounted");
  sdmmc_card_print_info(stdout, s_card);
  s_mounted = true;

  return ESP_OK;
}

esp_err_t sd_card_deinit(void) {
  if (!s_mounted) {
    return ESP_OK;
  }

  esp_err_t ret = esp_vfs_fat_sdcard_unmount(SD_MOUNT_POINT, s_card);
  if (ret == ESP_OK) {
    ESP_LOGI(TAG, "Card unmounted");
    s_mounted = false;
    s_card = NULL;

    if (s_pwr_ctrl_handle) {
      sd_pwr_ctrl_del_on_chip_ldo(s_pwr_ctrl_handle);
      s_pwr_ctrl_handle = NULL;
    }
  } else {
    ESP_LOGE(TAG, "Failed to unmount card (%s)", esp_err_to_name(ret));
  }
  return ret;
}

bool sd_card_is_mounted(void) { return s_mounted; }
