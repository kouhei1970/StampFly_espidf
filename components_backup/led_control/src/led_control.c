#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char *TAG = "led_control";

esp_err_t led_control_init(void)
{
    ESP_LOGI(TAG, "LED control module initialized");
    return ESP_OK;
}