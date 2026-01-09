  sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
  slot_config.width = 4;

  // On ESP32-P4, Slot 0 uses specific dedicated IOMUX pins.
  // We explicitly set them to match the Waveshare dev kit documentation.
  slot_config.clk = 43;
  slot_config.cmd = 44;
  slot_config.d0 = 39;
  slot_config.d1 = 40;
  slot_config.d2 = 41;
  slot_config.d3 = 42;
  slot_config.cd = GPIO_NUM_NC;
  slot_config.wp = GPIO_NUM_NC;
  slot_config.flags |= SDMMC_SLOT_FLAG_INTERNAL_PULLUP;
