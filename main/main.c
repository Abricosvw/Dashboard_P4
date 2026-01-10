  // 4. Initialize SD Card (~100 ms)
  // Load settings and assets.
  ESP_LOGI(TAG, "4. Initializing SD Card...");
  gpio_config_t sd_d3_conf = {
      .pin_bit_mask = (1ULL << 42),
      .mode = GPIO_MODE_INPUT,
      .pull_up_en = 1,
      .intr_type = GPIO_INTR_DISABLE,
  };
  gpio_config(&sd_d3_conf);

  if (sd_card_init() == ESP_OK) {
    ESP_LOGI(TAG, "   SD Card Mounted.");
    app_settings_init();
  } else {
    ESP_LOGW(TAG, "   SD Card failed/missing.");
  }

  // 5. Audio Init (~20 ms)
  // Initialize I2S and Codec. Now can access SD card for sounds.
  ESP_LOGI(TAG, "5. Initializing Audio...");
  audio_init();
  // Optional: Play boot sound here if available
  // audio_play_wav("/sdcard/boot.wav");
