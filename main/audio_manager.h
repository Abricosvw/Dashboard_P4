#pragma once

#include "esp_err.h"

// I2S Audio Pins (Waveshare ESP32-P4-Module-DEV-KIT)
#define AUDIO_BCLK_IO (12)
#define AUDIO_MCLK_IO (13)
#define AUDIO_WS_IO (10)
#define AUDIO_DOUT_IO (9) // DSDIN -> Input to Codec (Speaker Playback)
#define AUDIO_DIN_IO (11) // ASDOUT -> Output from Codec (Mic Recording)

// I2C Control Pins (Shared with Touch/Peripherals)
#define AUDIO_I2C_SDA (7)
#define AUDIO_I2C_SCL (8)
#define AUDIO_I2C_PORT (I2C_NUM_1) // Shared bus
#define AUDIO_I2C_ADDR (0x18)

// Amplifier Control
#define AUDIO_PA_ENABLE_IO (53)

// Audio Config
#define AUDIO_SAMPLE_RATE (16000)
#define AUDIO_BITS_PER_SAMPLE (16)

/**
 * @brief Initialize Audio System (I2C, I2S, Codec, Amp)
 * @return ESP_OK on success
 */
esp_err_t audio_init(void);

/**
 * @brief Play a test tone (Sine Wave)
 * @param freq_hz Frequency in Hz (e.g., 440)
 * @param duration_ms Duration in milliseconds
 * @return ESP_OK on success
 */
esp_err_t audio_play_tone(uint32_t freq_hz, uint32_t duration_ms);

/**
 * @brief Play a WAV file from filesystem
 * @param path Path to the WAV file (e.g., "/sdcard/path/to/file.wav")
 * @return ESP_OK on success
 */
esp_err_t audio_play_wav(const char *path);

/**
 * @brief Set output volume
 * @param volume_percent 0-100
 * @return ESP_OK on success
 */
esp_err_t audio_set_volume(int volume_percent);

/**
 * @brief Record audio to a WAV file
 * @param path Path to save the WAV file
 * @param duration_ms Duration to record in milliseconds
 * @return ESP_OK on success
 */
esp_err_t audio_record_wav(const char *path, uint32_t duration_ms);
