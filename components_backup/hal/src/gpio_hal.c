/*
 * GPIO HAL Implementation
 * 
 * 作成者: Kouhei Ito
 * ライセンス: MIT License
 * 
 * Copyright (c) 2025 Kouhei Ito
 */

#include "hal_common.h"
#include "esp_log.h"

static const char *TAG = "GPIO_HAL";

esp_err_t hal_init(void)
{
    ESP_LOGI(TAG, "HAL初期化開始");
    // TODO: HAL初期化実装
    ESP_LOGI(TAG, "HAL初期化完了");
    return ESP_OK;
}