/**
 * @file background_task.h
 * @brief Фоновые задачи для медленных операций (NVS, etc.) без блокировки UI
 */

#ifndef BACKGROUND_TASK_H
#define BACKGROUND_TASK_H

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

// Типы операций для фоновой обработки
typedef enum {
  BG_TASK_NVS_SAVE,      // Сохранение в NVS
  BG_TASK_SETTINGS_SAVE, // Сохранение настроек (NVS + SD)
  BG_TASK_NVS_LOAD,      // Загрузка из NVS
  BG_TASK_NVS_ERASE,     // Удаление из NVS
  BG_TASK_SYSTEM_RESET,  // Сброс системы
  BG_TASK_CUSTOM         // Пользовательская операция
} background_task_type_t;

// Структура фоновой задачи
typedef struct {
  background_task_type_t type; // Тип операции
  void *data;                  // Данные для операции
  size_t data_size;            // Размер данных
  void (*callback)(esp_err_t); // Callback функция по завершении
  void *callback_arg;          // Аргумент для callback
  TickType_t timeout;          // Таймаут операции
} background_task_t;

// Структура для NVS операций
typedef struct {
  const char *namespace; // NVS namespace
  const char *key;       // NVS ключ
  void *value;           // Значение для сохранения/загрузки
  size_t size;           // Размер значения
} nvs_operation_t;

/**
 * @brief Инициализация фоновой задачи
 * @return ESP_OK при успехе, иначе код ошибки
 */
esp_err_t background_task_init(void);

/**
 * @brief Деинициализация фоновой задачи
 */
void background_task_deinit(void);

/**
 * @brief Добавление задачи в очередь фоновой обработки
 * @param task Указатель на структуру задачи
 * @return ESP_OK при успехе, иначе код ошибки
 */
esp_err_t background_task_add(background_task_t *task);

/**
 * @brief Синхронное выполнение NVS операции сохранения
 * @param namespace NVS namespace
 * @param key NVS ключ
 * @param value Указатель на данные
 * @param size Размер данных
 * @return ESP_OK при успехе, иначе код ошибки
 */
esp_err_t background_nvs_save(const char *namespace, const char *key,
                              const void *value, size_t size);

/**
 * @brief Синхронное выполнение NVS операции загрузки
 * @param namespace NVS namespace
 * @param key NVS ключ
 * @param value Указатель на буфер для данных
 * @param size Размер буфера
 * @return ESP_OK при успехе, иначе код ошибки
 */
esp_err_t background_nvs_load(const char *namespace, const char *key,
                              void *value, size_t size);

/**
 * @brief Синхронное выполнение NVS операции удаления
 * @param namespace NVS namespace
 * @param key NVS ключ
 * @return ESP_OK при успехе, иначе код ошибки
 */
esp_err_t background_nvs_erase(const char *namespace, const char *key);

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
                                    void *callback_arg);

/**
 * @brief Получение статуса очереди задач
 * @param pending_count Указатель для количества ожидающих задач
 * @return ESP_OK при успехе, иначе код ошибки
 */
esp_err_t background_task_get_status(UBaseType_t *pending_count);

#endif // BACKGROUND_TASK_H
