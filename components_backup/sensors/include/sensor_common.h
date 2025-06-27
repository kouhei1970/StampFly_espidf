/*
 * Sensor Common Header
 * 
 * 作成者: Kouhei Ito
 * ライセンス: MIT License
 * 
 * Copyright (c) 2025 Kouhei Ito
 */

#ifndef SENSOR_COMMON_H
#define SENSOR_COMMON_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

// Sensors バージョン情報
#define SENSORS_VERSION_MAJOR 1
#define SENSORS_VERSION_MINOR 0
#define SENSORS_VERSION_PATCH 0

#ifdef __cplusplus
}
#endif

#endif // SENSOR_COMMON_H