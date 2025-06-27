/*
 * Sensor Fusion Module
 * 
 * 作成者: Kouhei Ito
 * ライセンス: MIT License
 * 
 * Copyright (c) 2025 Kouhei Ito
 */

#include "sensor_fusion.h"
#include "esp_log.h"

static const char* TAG = "SENSOR_FUSION";

esp_err_t sensor_fusion_init(void)
{
    ESP_LOGI(TAG, "センサーフュージョン初期化");
    return ESP_OK;
}

esp_err_t sensor_fusion_process(void)
{
    return ESP_OK;
}