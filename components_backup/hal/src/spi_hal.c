/*
 * HAL SPI Module
 * 
 * 作成者: Kouhei Ito
 * ライセンス: MIT License
 * 
 * Copyright (c) 2025 Kouhei Ito
 */

#include "hal_spi.h"
#include "esp_log.h"

static const char* TAG = "HAL_SPI";

esp_err_t hal_spi_init(void)
{
    ESP_LOGI(TAG, "SPI初期化");
    return ESP_OK;
}
