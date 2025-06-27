/*
 * HAL I2C Module
 * 
 * 作成者: Kouhei Ito
 * ライセンス: MIT License
 * 
 * Copyright (c) 2025 Kouhei Ito
 */

#include "hal_i2c.h"
#include "esp_log.h"

static const char* TAG = "HAL_I2C";

esp_err_t hal_i2c_init(void)
{
    ESP_LOGI(TAG, "I2C初期化");
    return ESP_OK;
}
