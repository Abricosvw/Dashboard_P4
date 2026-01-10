  // 3. Initialize SD Card (~100 ms)
  // Needed for UI assets, config, and startup sound.
  ESP_LOGI(TAG, "3. Initializing SD Card...");
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

  // 4. Audio Init (~20 ms)
  // Initialize I2S and Codec. Now can access SD card for sounds.
  ESP_LOGI(TAG, "4. Initializing Audio...");
  audio_init();
  // Optional: Play boot sound here if available
  // audio_play_wav("/sdcard/boot.wav");

  // 5. Initialize Display & GUI (~300-500 ms)
  // Priority: Get the screen showing something ASAP.
  ESP_LOGI(TAG, "5. Initializing Display & GUI...");
  board_init_backlight(i2c1_bus);
  board_set_backlight(0); // Keep backlight off during initialization to avoid artifacts

  board_init_touch(i2c1_bus); // Init touch driver

  esp_lcd_panel_handle_t panel = NULL;
  if (board_init_display(&panel) == ESP_OK) {
    // Initialize LVGL, create UI, start drawing task
    main_gui_init(panel);

    // Now that UI is active, turn on backlight
    vTaskDelay(pdMS_TO_TICKS(100)); // Allow one frame to render
    board_set_backlight(100);
    ESP_LOGI(TAG, "   Display is ON.");
  }
