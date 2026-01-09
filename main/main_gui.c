// Touchscreen read callback
static void touch_callback(lv_indev_drv_t *drv, lv_indev_data_t *data) {
  esp_lcd_touch_handle_t tp = (esp_lcd_touch_handle_t)drv->user_data;
  uint16_t x[1], y[1];
  uint8_t cnt;

  esp_lcd_touch_read_data(tp);
  bool touched = esp_lcd_touch_get_data(tp, x, y, NULL, &cnt, 1);

  if (touched && cnt > 0) {
    data->point.x = x[0];
    data->point.y = y[0];
    data->state = LV_INDEV_STATE_PRESSED;
  } else {
    data->state = LV_INDEV_STATE_RELEASED;
  }
}
