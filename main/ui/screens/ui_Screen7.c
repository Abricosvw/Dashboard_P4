#include "ui_Screen7.h"
#include "../ui.h"
#include "esp_random.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "ui_helpers.h"
#include "ui_screen_manager.h"
#include "wifi_controller.h"
#include <esp_log.h>
#include <stdio.h>

lv_obj_t *ui_Screen7;
static lv_obj_t *game_canvas;
static lv_timer_t *game_timer = NULL;
static lv_obj_t *label_status;

// Game State
#define GRID_W 40
#define GRID_H 24
#define CELL_SIZE 20

typedef struct {
  int x, y;
} Point;

static Point snake[100];
static int snake_len = 3;
static Point food;
static int dir_x = 1, dir_y = 0;
static bool game_over = false;
static int score = 0;

static void spawn_food() {
  food.x = esp_random() % GRID_W;
  food.y = esp_random() % GRID_H;
}

static void reset_game() {
  snake_len = 3;
  snake[0].x = 10;
  snake[0].y = 10;
  snake[1].x = 9;
  snake[1].y = 10;
  snake[2].x = 8;
  snake[2].y = 10;
  dir_x = 1;
  dir_y = 0;
  score = 0;
  game_over = false;
  spawn_food();
}

static void game_loop(lv_timer_t *timer) {
  // Only run game logic if Screen 7 is currently active
  if (lv_scr_act() != ui_Screen7) {
    return;
  }

  game_controller_state_t state;
  wifi_controller_get_state(&state);

  // Input Handling
  if (state.x > 50 && dir_x == 0) {
    dir_x = 1;
    dir_y = 0;
  } else if (state.x < -50 && dir_x == 0) {
    dir_x = -1;
    dir_y = 0;
  } else if (state.y > 50 && dir_y == 0) {
    dir_x = 0;
    dir_y = 1;
  } else if (state.y < -50 && dir_y == 0) {
    dir_x = 0;
    dir_y = -1;
  }

  if (state.button_start && game_over) {
    reset_game();
  }

  if (game_over)
    return;

  // Logic
  Point new_head = {snake[0].x + dir_x, snake[0].y + dir_y};

  // Collision with walls
  if (new_head.x < 0 || new_head.x >= GRID_W || new_head.y < 0 ||
      new_head.y >= GRID_H) {
    game_over = true;
    lv_label_set_text(label_status, "GAME OVER! Press START on Phone");
    return;
  }

  // Collision with self
  for (int i = 0; i < snake_len; i++) {
    if (new_head.x == snake[i].x && new_head.y == snake[i].y) {
      game_over = true;
      lv_label_set_text(label_status, "GAME OVER! Press START on Phone");
      return;
    }
  }

  // Move Snake
  for (int i = snake_len; i > 0; i--) {
    snake[i] = snake[i - 1];
  }
  snake[0] = new_head;

  // Eat Food
  if (new_head.x == food.x && new_head.y == food.y) {
    snake_len++;
    score += 10;
    spawn_food();
    char buf[64];
    snprintf(buf, sizeof(buf), "Score: %d | Connect to 'ESP32_GAME_CONTROLLER'",
             score);
    lv_label_set_text(label_status, buf);
  }

  // Draw
  // Clear canvas (fill black)
  lv_canvas_fill_bg(game_canvas, lv_color_hex(0x000000), LV_OPA_COVER);

  // Draw Snake
  lv_draw_rect_dsc_t rect_dsc;
  lv_draw_rect_dsc_init(&rect_dsc);
  rect_dsc.bg_color = lv_color_hex(0x00FF00);

  for (int i = 0; i < snake_len; i++) {
    lv_area_t coords = {.x1 = snake[i].x * CELL_SIZE,
                        .y1 = snake[i].y * CELL_SIZE,
                        .x2 = snake[i].x * CELL_SIZE + CELL_SIZE - 2,
                        .y2 = snake[i].y * CELL_SIZE + CELL_SIZE - 2};
    // Note: Canvas drawing is complex in LVGL 8/9.
    // For simplicity/performance in this demo, we might use simple obj creation
    // or just a single canvas with pixel manipulation. Let's use
    // lv_canvas_draw_rect if available or just fill buffer. Actually, creating
    // 100 objs is slow. Canvas is better.
    lv_canvas_draw_rect(game_canvas, coords.x1, coords.y1, CELL_SIZE - 2,
                        CELL_SIZE - 2, &rect_dsc);
  }

  // Draw Food
  rect_dsc.bg_color = lv_color_hex(0xFF0000);
  lv_canvas_draw_rect(game_canvas, food.x * CELL_SIZE, food.y * CELL_SIZE,
                      CELL_SIZE - 2, CELL_SIZE - 2, &rect_dsc);
}

// Buffer for canvas (move to PSRAM to avoid DRAM overflow)
static lv_color_t *cbuf = NULL;

void ui_Screen7_update_layout(void) {
  // Placeholder for layout updates
}

void ui_Screen7_screen_init(void) {
  ui_Screen7 = lv_obj_create(NULL);
  lv_obj_set_size(ui_Screen7, 1280, 720);
  lv_obj_set_pos(ui_Screen7, 0, 0);
  lv_obj_clear_flag(ui_Screen7, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_bg_color(ui_Screen7, lv_color_hex(0x1a1a1a), 0);

  // Canvas for Game
  // Allocate buffer in PSRAM
  size_t buf_size = LV_CANVAS_BUF_SIZE_TRUE_COLOR(1280, 720);
  cbuf = (lv_color_t *)heap_caps_malloc(buf_size, MALLOC_CAP_SPIRAM);
  if (!cbuf) {
    ESP_LOGE("SCREEN7", "Failed to allocate canvas buffer in PSRAM!");
    return;
  }

  game_canvas = lv_canvas_create(ui_Screen7);
  lv_canvas_set_buffer(game_canvas, cbuf, 1280, 720, LV_IMG_CF_TRUE_COLOR);
  lv_obj_align(game_canvas, LV_ALIGN_CENTER, 0, 0);
  lv_canvas_fill_bg(game_canvas, lv_color_hex(0x000000), LV_OPA_COVER);

  // Status Label
  label_status = lv_label_create(ui_Screen7);
  lv_label_set_text(label_status,
                    "Connect WiFi: 'ESP32_GAME_CONTROLLER' (No Pass)");
  lv_obj_set_style_text_color(label_status, lv_color_white(), 0);
  lv_obj_set_style_text_font(label_status, &lv_font_montserrat_24, 0);
  lv_obj_align(label_status, LV_ALIGN_TOP_MID, 0, 10);

  reset_game();
  game_timer = lv_timer_create(game_loop, 100, NULL); // 10 FPS

  ESP_LOGI("SCREEN7", "Screen 7 (Game) initialized");
}

void ui_Screen7_screen_destroy(void) {
  if (game_timer) {
    lv_timer_del(game_timer);
    game_timer = NULL;
  }
  if (ui_Screen7) {
    lv_obj_del(ui_Screen7);
    ui_Screen7 = NULL;
  }
  if (cbuf) {
    free(cbuf);
    cbuf = NULL;
  }
}
