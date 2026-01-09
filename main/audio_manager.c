#include "audio_manager.h"
#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "driver/i2s_std.h"
#include "es8311.h"
#include "esp_check.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

static const char *TAG = "AUDIO_MGR";
extern i2c_master_bus_handle_t i2c1_bus;
static i2s_chan_handle_t tx_handle = NULL;
static i2s_chan_handle_t rx_handle = NULL;
static uint32_t current_sample_rate = AUDIO_SAMPLE_RATE;

// Forward declaration
esp_err_t audio_set_sample_rate_internal(uint32_t rate);

// I2C initialization is handled globally in main.c using i2c_bus component

static es8311_handle_t s_es_handle = NULL;

static esp_err_t audio_codec_init(void) {
  // Initialize ES8311 Codec
  s_es_handle = es8311_create(AUDIO_I2C_PORT, ES8311_ADDRESS_0);
  ESP_RETURN_ON_FALSE(s_es_handle, ESP_FAIL, TAG, "ES8311 create failed");

  const es8311_clock_config_t clk_cfg = {
      .mclk_from_mclk_pin = true,
      .mclk_frequency = current_sample_rate * 256,
      .sample_frequency = current_sample_rate};
  ESP_RETURN_ON_ERROR(es8311_init(s_es_handle, &clk_cfg, ES8311_RESOLUTION_16,
                                  ES8311_RESOLUTION_16),
                      TAG, "ES8311 init failed");

  ESP_RETURN_ON_ERROR(es8311_voice_volume_set(s_es_handle, 65, NULL), TAG,
                      "ES8311 volume set failed"); // Set volume to 65%
  ESP_RETURN_ON_ERROR(es8311_microphone_config(s_es_handle, false), TAG,
                      "ES8311 mic config failed"); // false = not digital mic
  ESP_RETURN_ON_ERROR(es8311_voice_mute(s_es_handle, false), TAG,
                      "ES8311 unmute failed"); // Explicit Unmute

  return ESP_OK;
}

static esp_err_t audio_i2s_init(void) {
  i2s_chan_config_t chan_cfg =
      I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
  ESP_RETURN_ON_ERROR(i2s_new_channel(&chan_cfg, &tx_handle, &rx_handle), TAG,
                      "I2S new channel failed");

  i2s_std_config_t std_cfg = {
      .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(current_sample_rate),
      .slot_cfg = I2S_STD_MSB_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT,
                                                  I2S_SLOT_MODE_STEREO),
      .gpio_cfg =
          {
              .mclk = AUDIO_MCLK_IO,
              .bclk = AUDIO_BCLK_IO,
              .ws = AUDIO_WS_IO,
              .dout = AUDIO_DOUT_IO,
              .din = AUDIO_DIN_IO,
          },
  };
  // Enable MCLK output
  std_cfg.clk_cfg.mclk_multiple = I2S_MCLK_MULTIPLE_256;

  ESP_RETURN_ON_ERROR(i2s_channel_init_std_mode(tx_handle, &std_cfg), TAG,
                      "I2S TX init failed");
  ESP_RETURN_ON_ERROR(i2s_channel_init_std_mode(rx_handle, &std_cfg), TAG,
                      "I2S RX init failed");

  ESP_RETURN_ON_ERROR(i2s_channel_enable(tx_handle), TAG,
                      "I2S TX enable failed");
  ESP_RETURN_ON_ERROR(i2s_channel_enable(rx_handle), TAG,
                      "I2S RX enable failed");

  return ESP_OK;
}

// Helper to switch sample rate dynamically
esp_err_t audio_set_sample_rate_internal(uint32_t rate) {
  if (rate == current_sample_rate)
    return ESP_OK;

  ESP_LOGI(TAG, "Switching Sample Rate: %" PRIu32 " Hz -> %" PRIu32 " Hz",
           current_sample_rate, rate);

  // 1. Disable I2S
  i2s_channel_disable(tx_handle);

  // 2. Reconfigure I2S Clock
  i2s_std_clk_config_t clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(rate);
  clk_cfg.mclk_multiple = I2S_MCLK_MULTIPLE_256;
  ESP_RETURN_ON_ERROR(i2s_channel_reconfig_std_clock(tx_handle, &clk_cfg), TAG,
                      "I2S reconfig failed");

  // 3. Reconfigure Codec
  es8311_clock_config_t codec_clk_cfg = {.mclk_from_mclk_pin = true,
                                         .mclk_frequency = rate * 256,
                                         .sample_frequency = rate};
  ESP_RETURN_ON_ERROR(es8311_init(s_es_handle, &codec_clk_cfg,
                                  ES8311_RESOLUTION_16, ES8311_RESOLUTION_16),
                      TAG, "ES8311 re-init failed");

  // 4. Enable I2S
  i2s_channel_enable(tx_handle);

  current_sample_rate = rate;
  return ESP_OK;
}

esp_err_t audio_init(void) {
  ESP_LOGI(TAG, "Initializing Audio System (Variant 1)...");

  // 1. Init I2S (This starts MCLK)
  ESP_RETURN_ON_ERROR(audio_i2s_init(), TAG, "I2S Init Failed");

  // 2. Wait for MCLK and power to stabilize
  vTaskDelay(pdMS_TO_TICKS(100));

  // 3. Init Codec
  ESP_LOGI(TAG, "Initializing ES8311 codec at address 0x%02X...",
           ES8311_ADDRESS_0);
  ESP_RETURN_ON_ERROR(audio_codec_init(), TAG, "Codec Init Failed");

  // 4. Power Amplifier is already enabled in app_main (GPIO 53)
  vTaskDelay(pdMS_TO_TICKS(100));

  ESP_LOGI(TAG, "Audio System Initialized Successfully");
  return ESP_OK;
}

esp_err_t audio_play_tone(uint32_t freq_hz, uint32_t duration_ms) {
  if (!tx_handle)
    return ESP_FAIL;

  ESP_LOGI(TAG, "Playing test tone: %" PRIu32 " Hz for %" PRIu32 " ms", freq_hz,
           duration_ms);

  size_t bytes_written = 0;
  uint32_t sample_rate = AUDIO_SAMPLE_RATE;
  uint32_t samples = (sample_rate * duration_ms) / 1000;
  int16_t *buffer = malloc(samples * 2 * sizeof(int16_t)); // Stereo

  if (!buffer) {
    ESP_LOGE(TAG, "Failed to allocate audio buffer");
    return ESP_ERR_NO_MEM;
  }

  for (int i = 0; i < samples; i++) {
    double t = (double)i / sample_rate;
    int16_t val =
        (int16_t)(sin(2 * M_PI * freq_hz * t) * 15000); // Amplitude 15000/32767
    buffer[i * 2] = val;                                // Left
    buffer[i * 2 + 1] = val;                            // Right
  }

  i2s_channel_write(tx_handle, buffer, samples * 2 * sizeof(int16_t),
                    &bytes_written, 1000);

  free(buffer);
  return ESP_OK;
}

#include <string.h>

// ... (existing includes)

// WAV Header Structure (simplified for parsing)
typedef struct {
  char riff_header[4]; // "RIFF"
  uint32_t wav_size;
  char wave_header[4]; // "WAVE"
  char fmt_header[4];  // "fmt "
  uint32_t fmt_chunk_size;
  uint16_t audio_format;
  uint16_t num_channels;
  uint32_t sample_rate;
  uint32_t byte_rate;
  uint16_t sample_alignment;
  uint16_t bit_depth;
  char data_header[4]; // "data"
  uint32_t data_bytes;
} wav_header_t;

esp_err_t audio_play_wav(const char *path) {
  if (!tx_handle) {
    ESP_LOGE(TAG, "I2S not initialized");
    return ESP_FAIL;
  }

  ESP_LOGI(TAG, "Playing WAV file: %s", path);
  FILE *f = fopen(path, "rb");
  if (f == NULL) {
    ESP_LOGE(TAG, "Failed to open WAV file: %s", path);
    return ESP_FAIL;
  }

  // Read Header
  wav_header_t header;
  if (fread(&header, 1, sizeof(wav_header_t), f) != sizeof(wav_header_t)) {
    ESP_LOGE(TAG, "Failed to read WAV header");
    fclose(f);
    return ESP_FAIL;
  }

  // Debug Output
  ESP_LOGI(TAG, "WAV Header Details:");
  ESP_LOGI(TAG, "  RIFF: %.4s", header.riff_header);
  ESP_LOGI(TAG, "  WAVE: %.4s", header.wave_header);
  ESP_LOGI(TAG, "  Format: %d (1=PCM)", header.audio_format);
  ESP_LOGI(TAG, "  Channels: %d", header.num_channels);
  ESP_LOGI(TAG, "  Sample Rate: %" PRIu32 " Hz", header.sample_rate);
  ESP_LOGI(TAG, "  Bit Depth: %d bits", header.bit_depth);
  ESP_LOGI(TAG, "  Data Bytes: %" PRIu32, header.data_bytes);

  // Validate basic PCM
  if (strncmp(header.riff_header, "RIFF", 4) != 0 ||
      strncmp(header.wave_header, "WAVE", 4) != 0) {
    ESP_LOGE(TAG, "Invalid WAV file format");
    fclose(f);
    return ESP_FAIL;
  }

  // Auto-switch sample rate if supported (added)
  if (header.sample_rate > 0 && header.sample_rate <= 48000) {
    audio_set_sample_rate_internal(header.sample_rate);
  } else {
    ESP_LOGW(TAG, "Unsupported WAV sample rate: %" PRIu32, header.sample_rate);
  }

  // Search for "data" chunk
  uint32_t chunk_id;
  uint32_t wav_chunk_size;
  bool data_found = false;

  // Start searching after standard 36-byte header (RIFF + WAVE + fmt basic)
  // We already read sizeof(wav_header_t) which is 44 bytes.
  // Ideally we should traverse from the beginning, but let's reset to after
  // "fmt " chunk if possible. A standard "fmt " chunk is 16 bytes.
  // header.fmt_chunk_size tells us actual size.

  long current_offset = 12; // Start after RIFF + Size + WAVE (4+4+4)
  fseek(f, current_offset, SEEK_SET);

  while (fread(&chunk_id, 1, 4, f) == 4 &&
         fread(&wav_chunk_size, 1, 4, f) == 4) {
    if (chunk_id ==
        0x61746164) { // "data" in little endian is 0x61746164 ('d','a','t','a')
      data_found = true;
      ESP_LOGI(TAG, "Found data chunk at offset %ld, size: %" PRIu32, ftell(f),
               wav_chunk_size);
      header.data_bytes = wav_chunk_size;
      break;
    }

    // Skip this chunk
    fseek(f, wav_chunk_size, SEEK_CUR);
  }

  if (!data_found) {
    ESP_LOGE(TAG, "WAV 'data' chunk not found");
    fclose(f);
    // Fallback: try standard 44 bytes if search failed, though unlikely to work
    // well
    return ESP_FAIL;
  }

  // File pointer is now at the beginning of data samples
  // long data_start_pos = ftell(f); // Unused

  // Buffer for reading
  const size_t chunk_size = 1024;
  int16_t *buffer = malloc(chunk_size);
  if (!buffer) {
    ESP_LOGE(TAG, "Memory allocation failed");
    fclose(f);
    return ESP_ERR_NO_MEM;
  }

  size_t bytes_read = 0;
  size_t bytes_written = 0;
  size_t total_bytes_played = 0;

  // Play only the data chunk
  while (total_bytes_played < header.data_bytes) {
    size_t bytes_to_read = chunk_size;

    if (total_bytes_played + chunk_size > header.data_bytes) {
      bytes_to_read = header.data_bytes - total_bytes_played;
    }

    bytes_read = fread(buffer, 1, bytes_to_read, f);
    if (bytes_read == 0)
      break; // EOF or error

    i2s_channel_write(tx_handle, buffer, bytes_read, &bytes_written, 1000);
    total_bytes_played += bytes_read;
  }

  free(buffer);
  fclose(f);
  ESP_LOGI(TAG, "WAV playback finished");
  return ESP_OK;
}

esp_err_t audio_set_volume(int volume_percent) {
  if (s_es_handle) {
    return es8311_voice_volume_set(s_es_handle, volume_percent, NULL);
  }
  return ESP_FAIL;
}

static esp_err_t audio_codec_enable_adc(void) {
  if (!s_es_handle)
    return ESP_FAIL;
  // Set ADC gain (Replaced invalid function with correct one)
  es8311_microphone_gain_set(s_es_handle, ES8311_MIC_GAIN_18DB);
  return ESP_OK;
}

esp_err_t audio_record_wav(const char *path, uint32_t duration_ms) {
  if (!rx_handle) {
    ESP_LOGE(TAG, "I2S RX not initialized");
    return ESP_FAIL;
  }

  ESP_LOGI(TAG, "Recording to %s for %" PRIu32 " ms", path, duration_ms);
  FILE *f = fopen(path, "wb");
  if (!f) {
    ESP_LOGE(TAG, "Failed to open file for writing");
    return ESP_FAIL;
  }

  // WAV Header placeholder
  wav_header_t header;
  memset(&header, 0, sizeof(wav_header_t));
  fwrite(&header, 1, sizeof(wav_header_t), f); // Reserve space

  // Ensure Codec ADC is ready
  audio_codec_enable_adc();

  size_t chunk_size = 1024;
  int16_t *buffer = malloc(chunk_size);
  if (!buffer) {
    fclose(f);
    return ESP_ERR_NO_MEM;
  }

  size_t bytes_read = 0;
  size_t total_bytes = 0;
  uint32_t start_time = xTaskGetTickCount();
  uint32_t target_ticks = pdMS_TO_TICKS(duration_ms);

  while ((xTaskGetTickCount() - start_time) < target_ticks) {
    if (i2s_channel_read(rx_handle, buffer, chunk_size, &bytes_read, 100) ==
        ESP_OK) {
      fwrite(buffer, 1, bytes_read, f);
      total_bytes += bytes_read;
    } else {
      vTaskDelay(1); // Yield if no data
    }
  }

  free(buffer);

  // Finalize WAV Header
  memcpy(header.riff_header, "RIFF", 4);
  header.wav_size = 36 + total_bytes;
  memcpy(header.wave_header, "WAVE", 4);
  memcpy(header.fmt_header, "fmt ", 4);
  header.fmt_chunk_size = 16;
  header.audio_format = 1; // PCM
  header.num_channels = 2; // Stereo (I2S config is stereo)
  header.sample_rate = current_sample_rate;
  header.bit_depth = 16;
  header.byte_rate =
      current_sample_rate * 2 * 2; // Rate * Channels * BytesPerSample
  header.sample_alignment = 4;
  memcpy(header.data_header, "data", 4);
  header.data_bytes = total_bytes;

  fseek(f, 0, SEEK_SET);
  fwrite(&header, 1, sizeof(wav_header_t), f);
  fclose(f);

  ESP_LOGI(TAG, "Recording complete. Size: %u bytes", total_bytes);
  return ESP_OK;
}
