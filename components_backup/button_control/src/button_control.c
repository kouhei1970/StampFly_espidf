/*
 * Button Control Module
 * 
 * 作成者: Kouhei Ito
 * ライセンス: MIT License
 * 
 * Copyright (c) 2025 Kouhei Ito
 */

#include "button_control.h"
#include "esp_log.h"

static const char* TAG = "BUTTON_CTRL";

esp_err_t button_control_init(void)
{
    ESP_LOGI(TAG, "ボタン制御初期化");
    return ESP_OK;
}

bool button_is_pressed(button_id_t button_id)
{
    return false;
}