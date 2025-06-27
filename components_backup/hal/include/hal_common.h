/*
 * HAL Common Header - Hardware Abstraction Layer
 * 
 * 作成者: Kouhei Ito
 * ライセンス: MIT License
 * 
 * Copyright (c) 2025 Kouhei Ito
 */

#ifndef HAL_COMMON_H
#define HAL_COMMON_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

// HAL バージョン情報
#define HAL_VERSION_MAJOR 1
#define HAL_VERSION_MINOR 0
#define HAL_VERSION_PATCH 0

// HAL 初期化関数
esp_err_t hal_init(void);

#ifdef __cplusplus
}
#endif

#endif // HAL_COMMON_H