/*
 * Position Estimation Module
 * 
 * 作成者: Kouhei Ito
 * ライセンス: MIT License
 * 
 * Copyright (c) 2025 Kouhei Ito
 */

#include "position_estimation.h"
#include "esp_log.h"

static const char* TAG = "POS_EST";

esp_err_t position_estimation_init(void)
{
    ESP_LOGI(TAG, "位置推定初期化");
    return ESP_OK;
}

esp_err_t position_estimation_update(void)
{
    return ESP_OK;
}