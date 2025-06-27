#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char *TAG = "navigation";

esp_err_t navigation_init(void)
{
    ESP_LOGI(TAG, "Navigation module initialized");
    return ESP_OK;
}