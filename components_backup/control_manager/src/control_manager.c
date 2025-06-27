/*
 * Control Manager Module
 * 
 * 作成者: Kouhei Ito
 * ライセンス: MIT License
 * 
 * Copyright (c) 2025 Kouhei Ito
 */

#include "control_manager.h"
#include "esp_log.h"

static const char* TAG = "CTRL_MGR";

esp_err_t control_manager_init(void)
{
    ESP_LOGI(TAG, "制御マネージャー初期化");
    return ESP_OK;
}

esp_err_t control_manager_set_mode(control_mode_t mode)
{
    ESP_LOGI(TAG, "制御モード変更: %d", mode);
    return ESP_OK;
}