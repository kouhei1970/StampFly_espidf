/*
 * CLI Interface Module
 * 
 * 作成者: Kouhei Ito
 * ライセンス: MIT License
 * 
 * Copyright (c) 2025 Kouhei Ito
 */

#include "cli_interface.h"
#include "esp_log.h"

static const char* TAG = "CLI";

esp_err_t cli_interface_init(void)
{
    ESP_LOGI(TAG, "CLIインターフェース初期化");
    return ESP_OK;
}

esp_err_t cli_process_command(const char* command)
{
    ESP_LOGI(TAG, "コマンド処理: %s", command);
    return ESP_OK;
}