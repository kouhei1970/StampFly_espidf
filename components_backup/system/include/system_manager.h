/*
 * System Manager Header
 * 
 * 作成者: Kouhei Ito
 * ライセンス: MIT License
 * 
 * Copyright (c) 2025 Kouhei Ito
 */

#ifndef SYSTEM_MANAGER_H
#define SYSTEM_MANAGER_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

// System バージョン情報
#define SYSTEM_VERSION_MAJOR 1
#define SYSTEM_VERSION_MINOR 0
#define SYSTEM_VERSION_PATCH 0

// システム初期化関数
esp_err_t system_manager_init(void);

#ifdef __cplusplus
}
#endif

#endif // SYSTEM_MANAGER_H