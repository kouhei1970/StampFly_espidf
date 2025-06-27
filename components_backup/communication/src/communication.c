#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char *TAG = "communication";

esp_err_t communication_init(void)
{
    ESP_LOGI(TAG, "Communication module initialized");
    return ESP_OK;
}