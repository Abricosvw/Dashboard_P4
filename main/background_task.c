/**
 * @file background_task.c
 * @brief Реализация фоновых задач для медленных операций
 */

#include "background_task.h"
#include "esp_log.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "ui/settings_config.h" // For settings_save()
#include <string.h>


static const char *TAG = "BACKGROUND_TASK";

// Очередь для фоновых задач
static QueueHandle_t background_queue = NULL;

// Задача FreeRTOS для обработки фоновых операций
static TaskHandle_t background_task_handle = NULL;

// Статические буферы для избежания malloc в background_nvs_save_async
static uint8_t static_data_buffer[256];
static nvs_operation_t static_nvs_operation;

// Максимальный размер очереди
#define BACKGROUND_QUEUE_SIZE 10

// Размер стека для фоновой задачи
#define BACKGROUND_TASK_STACK_SIZE 4096

// Приоритет фоновой задачи
#define BACKGROUND_TASK_PRIORITY 5

/**
 * @brief Основная функция фоновой задачи
 * @param pvParameters Параметры задачи (не используются)
 */
static void background_task_worker(void *pvParameters) {
  background_task_t task;

  ESP_LOGI(TAG, "Background task worker started");

  while (1) {
    // Ожидание задачи из очереди
    if (xQueueReceive(background_queue, &task, portMAX_DELAY) == pdTRUE) {
      esp_err_t result = ESP_OK;

      ESP_LOGD(TAG, "Processing background task type: %d", task.type);

      switch (task.type) {
      case BG_TASK_NVS_SAVE: {
        nvs_operation_t *nvs_op = (nvs_operation_t *)task.data;
        if (nvs_op) {
          nvs_handle_t nvs_handle;
          result = nvs_open(nvs_op->namespace, NVS_READWRITE, &nvs_handle);
          if (result == ESP_OK) {
            result = nvs_set_blob(nvs_handle, nvs_op->key, nvs_op->value,
                                  nvs_op->size);
            if (result == ESP_OK) {
              result = nvs_commit(nvs_handle);
            }
            nvs_close(nvs_handle);
          }

          // Освобождаем память только если использовалась динамическая память
          // Статические буферы не освобождаем
          if (nvs_op != &static_nvs_operation) {
            // Использовалась динамическая память
            free(nvs_op->value);
            free(nvs_op);
          }
          // Для статической памяти ничего не делаем - она будет
          // переиспользована
        }
        break;
      }

      case BG_TASK_SETTINGS_SAVE: {
        if (task.data) {
          settings_save((const touch_settings_t *)task.data);
          // The data was malloc'd in trigger_settings_save, so we must free it
          // here.
          free(task.data);
        } else {
          ESP_LOGE(TAG, "BG_TASK_SETTINGS_SAVE received null data!");
          result = ESP_ERR_INVALID_ARG;
        }
        break;
      }

      case BG_TASK_NVS_LOAD: {
        nvs_operation_t *nvs_op = (nvs_operation_t *)task.data;
        if (nvs_op) {
          nvs_handle_t nvs_handle;
          result = nvs_open(nvs_op->namespace, NVS_READONLY, &nvs_handle);
          if (result == ESP_OK) {
            result = nvs_get_blob(nvs_handle, nvs_op->key, nvs_op->value,
                                  &nvs_op->size);
            nvs_close(nvs_handle);
          }
          free(nvs_op);
        }
        break;
      }

      case BG_TASK_NVS_ERASE: {
        nvs_operation_t *nvs_op = (nvs_operation_t *)task.data;
        if (nvs_op) {
          nvs_handle_t nvs_handle;
          result = nvs_open(nvs_op->namespace, NVS_READWRITE, &nvs_handle);
          if (result == ESP_OK) {
            result = nvs_erase_key(nvs_handle, nvs_op->key);
            if (result == ESP_OK) {
              result = nvs_commit(nvs_handle);
            }
            nvs_close(nvs_handle);
          }
          free(nvs_op);
        }
        break;
      }

      case BG_TASK_SYSTEM_RESET: {
        ESP_LOGI(TAG, "System reset requested");
        // Здесь можно добавить дополнительную логику перед перезагрузкой
        vTaskDelay(pdMS_TO_TICKS(100)); // Небольшая задержка
        esp_restart();
        break;
      }

      case BG_TASK_CUSTOM: {
        // Пользовательская операция - пока не реализована
        ESP_LOGW(TAG, "Custom background task not implemented");
        result = ESP_ERR_NOT_SUPPORTED;
        break;
      }

      default:
        ESP_LOGW(TAG, "Unknown background task type: %d", task.type);
        result = ESP_ERR_INVALID_ARG;
        break;
      }

      // Вызов callback функции если она указана
      if (task.callback) {
        task.callback(result);
      }

      ESP_LOGD(TAG, "Background task completed with result: %s",
               esp_err_to_name(result));
    }
  }
}

/**
 * @brief Инициализация фоновой задачи
 * @return ESP_OK при успехе, иначе код ошибки
 */
esp_err_t background_task_init(void) {
  ESP_LOGI(TAG, "Initializing background task system");

  // Создание очереди для задач
  background_queue =
      xQueueCreate(BACKGROUND_QUEUE_SIZE, sizeof(background_task_t));
  if (background_queue == NULL) {
    ESP_LOGE(TAG, "Failed to create background task queue");
    return ESP_ERR_NO_MEM;
  }

  // Создание фоновой задачи
  BaseType_t ret = xTaskCreate(
      background_task_worker, "bg_worker", BACKGROUND_TASK_STACK_SIZE, NULL,
      BACKGROUND_TASK_PRIORITY, &background_task_handle);

  if (ret != pdPASS) {
    ESP_LOGE(TAG, "Failed to create background task");
    vQueueDelete(background_queue);
    background_queue = NULL;
    return ESP_ERR_NO_MEM;
  }

  ESP_LOGI(TAG, "Background task system initialized successfully");
  return ESP_OK;
}

/**
 * @brief Деинициализация фоновой задачи
 */
void background_task_deinit(void) {
  ESP_LOGI(TAG, "Deinitializing background task system");

  if (background_task_handle) {
    vTaskDelete(background_task_handle);
    background_task_handle = NULL;
  }

  if (background_queue) {
    vQueueDelete(background_queue);
    background_queue = NULL;
  }
}

/**
 * @brief Добавление задачи в очередь фоновой обработки
 * @param task Указатель на структуру задачи
 * @return ESP_OK при успехе, иначе код ошибки
 */
esp_err_t background_task_add(background_task_t *task) {
  if (!background_queue || !task) {
    return ESP_ERR_INVALID_ARG;
  }

  // Убираем блокировку - если очередь полна, возвращаем ошибку немедленно
  if (xQueueSend(background_queue, task, 0) != pdTRUE) {
    ESP_LOGW(TAG, "Failed to add task to background queue - queue full");
    return ESP_ERR_TIMEOUT;
  }

  return ESP_OK;
}

/**
 * @brief Синхронное выполнение NVS операции сохранения
 * @param namespace NVS namespace
 * @param key NVS ключ
 * @param value Указатель на данные
 * @param size Размер данных
 * @return ESP_OK при успехе, иначе код ошибки
 */
esp_err_t background_nvs_save(const char *namespace, const char *key,
                              const void *value, size_t size) {
  if (!namespace || !key || !value || size == 0) {
    return ESP_ERR_INVALID_ARG;
  }

  nvs_handle_t nvs_handle;
  esp_err_t ret = nvs_open(namespace, NVS_READWRITE, &nvs_handle);
  if (ret != ESP_OK) {
    return ret;
  }

  ret = nvs_set_blob(nvs_handle, key, value, size);
  if (ret == ESP_OK) {
    ret = nvs_commit(nvs_handle);
  }

  nvs_close(nvs_handle);
  return ret;
}

/**
 * @brief Синхронное выполнение NVS операции загрузки
 * @param namespace NVS namespace
 * @param key NVS ключ
 * @param value Указатель на буфер для данных
 * @param size Размер буфера
 * @return ESP_OK при успехе, иначе код ошибки
 */
esp_err_t background_nvs_load(const char *namespace, const char *key,
                              void *value, size_t size) {
  if (!namespace || !key || !value || size == 0) {
    return ESP_ERR_INVALID_ARG;
  }

  nvs_handle_t nvs_handle;
  esp_err_t ret = nvs_open(namespace, NVS_READONLY, &nvs_handle);
  if (ret != ESP_OK) {
    return ret;
  }

  ret = nvs_get_blob(nvs_handle, key, value, &size);
  nvs_close(nvs_handle);
  return ret;
}

/**
 * @brief Синхронное выполнение NVS операции удаления
 * @param namespace NVS namespace
 * @param key NVS ключ
 * @return ESP_OK при успехе, иначе код ошибки
 */
esp_err_t background_nvs_erase(const char *namespace, const char *key) {
  if (!namespace || !key) {
    return ESP_ERR_INVALID_ARG;
  }

  nvs_handle_t nvs_handle;
  esp_err_t ret = nvs_open(namespace, NVS_READWRITE, &nvs_handle);
  if (ret != ESP_OK) {
    return ret;
  }

  ret = nvs_erase_key(nvs_handle, key);
  if (ret == ESP_OK) {
    ret = nvs_commit(nvs_handle);
  }

  nvs_close(nvs_handle);
  return ret;
}

/**
 * @brief Асинхронное выполнение NVS операции сохранения
 * @param namespace NVS namespace
 * @param key NVS ключ
 * @param value Указатель на данные
 * @param size Размер данных
 * @param callback Callback функция по завершении (может быть NULL)
 * @param callback_arg Аргумент для callback
 * @return ESP_OK при успехе, иначе код ошибки
 */
esp_err_t background_nvs_save_async(const char *namespace, const char *key,
                                    const void *value, size_t size,
                                    void (*callback)(esp_err_t),
                                    void *callback_arg) {
  if (!namespace || !key || !value || size == 0) {
    return ESP_ERR_INVALID_ARG;
  }

  void *value_copy;
  nvs_operation_t *nvs_op;

  if (size <= sizeof(static_data_buffer)) {
    // Используем статический буфер для избежания malloc
    value_copy = static_data_buffer;
    nvs_op = &static_nvs_operation;
    memcpy(value_copy, value, size);
  } else {
    // Для больших данных используем динамическую память
    value_copy = malloc(size);
    if (!value_copy) {
      return ESP_ERR_NO_MEM;
    }
    memcpy(value_copy, value, size);

    nvs_op = malloc(sizeof(nvs_operation_t));
    if (!nvs_op) {
      free(value_copy);
      return ESP_ERR_NO_MEM;
    }
  }

  // Заполнение структуры операции
  nvs_op->namespace = namespace;
  nvs_op->key = key;
  nvs_op->value = value_copy;
  nvs_op->size = size;

  // Создание фоновой задачи
  background_task_t task = {
      .type = BG_TASK_NVS_SAVE,
      .data = nvs_op,
      .data_size = sizeof(nvs_operation_t),
      .callback = callback,
      .callback_arg = callback_arg,
      .timeout = pdMS_TO_TICKS(5000) // 5 секунд таймаут
  };

  esp_err_t result = background_task_add(&task);

  // Если задача не была добавлена и мы использовали динамическую память,
  // освобождаем её
  if (result != ESP_OK && size > sizeof(static_data_buffer)) {
    // Освобождаем динамическую память при ошибке
    free(value_copy);
    free(nvs_op);
  }

  return result;
}

/**
 * @brief Получение статуса очереди задач
 * @param pending_count Указатель для количества ожидающих задач
 * @return ESP_OK при успехе, иначе код ошибки
 */
esp_err_t background_task_get_status(UBaseType_t *pending_count) {
  if (!pending_count) {
    return ESP_ERR_INVALID_ARG;
  }

  if (!background_queue) {
    return ESP_ERR_INVALID_STATE;
  }

  *pending_count = uxQueueMessagesWaiting(background_queue);
  return ESP_OK;
}
