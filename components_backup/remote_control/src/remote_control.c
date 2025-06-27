#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char *TAG = "remote_control";

esp_err_t remote_control_init(void)
{
    ESP_LOGI(TAG, "Remote control module initialized");
    return ESP_OK;
}