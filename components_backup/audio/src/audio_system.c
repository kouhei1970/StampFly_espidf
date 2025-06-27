/*
 * Audio System Module
 * 
 * 作成者: Kouhei Ito
 * ライセンス: MIT License
 * 
 * Copyright (c) 2025 Kouhei Ito
 */

#include "audio_system.h"
#include "esp_log.h"

static const char* TAG = "AUDIO_SYSTEM";

esp_err_t audio_system_init(void)
{
    ESP_LOGI(TAG, "オーディオシステム初期化");
    return ESP_OK;
}

esp_err_t audio_play_tone(uint16_t frequency, uint16_t duration)
{
    ESP_LOGI(TAG, "トーン再生: %dHz, %dms", frequency, duration);
    return ESP_OK;
}