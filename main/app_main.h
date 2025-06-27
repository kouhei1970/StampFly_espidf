/*
 * StampFly ESP-IDF版 メインアプリケーション ヘッダーファイル
 * 
 * ESP32-S3ベースドローン制御システム
 * 
 * 作成者: Kouhei Ito
 * ライセンス: MIT License
 * 
 * Copyright (c) 2025 Kouhei Ito
 */

#ifndef APP_MAIN_H
#define APP_MAIN_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/timers.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

// プロジェクト情報
#define STAMPFLY_PROJECT_NAME    "StampFly ESP-IDF"
#define STAMPFLY_VERSION_MAJOR   1
#define STAMPFLY_VERSION_MINOR   0
#define STAMPFLY_VERSION_PATCH   0
#define STAMPFLY_VERSION_STRING  "1.0.0"

// システム設定
#define MAIN_TASK_PRIORITY           (configMAX_PRIORITIES - 1)
#define MAIN_TASK_STACK_SIZE         (8192)
#define MAIN_TASK_CORE_ID           (1)

#define CONTROL_TASK_PRIORITY       (configMAX_PRIORITIES - 2)
#define CONTROL_TASK_STACK_SIZE     (8192)
#define CONTROL_TASK_CORE_ID        (1)

#define SENSOR_TASK_PRIORITY        (configMAX_PRIORITIES - 3)
#define SENSOR_TASK_STACK_SIZE      (6144)
#define SENSOR_TASK_CORE_ID         (0)

#define CLI_TASK_PRIORITY           (3)
#define CLI_TASK_STACK_SIZE         (4096)
#define CLI_TASK_CORE_ID            (0)

// タスクハンドル
extern TaskHandle_t main_task_handle;
extern TaskHandle_t control_task_handle;
extern TaskHandle_t sensor_task_handle;
extern TaskHandle_t cli_task_handle;

// システム状態
typedef enum {
    SYSTEM_STATE_INIT = 0,      // システム初期化中
    SYSTEM_STATE_CALIBRATION,   // キャリブレーション中
    SYSTEM_STATE_READY,         // 準備完了
    SYSTEM_STATE_ARMED,         // アーム済み
    SYSTEM_STATE_FLIGHT,        // 飛行中
    SYSTEM_STATE_EMERGENCY,     // 緊急状態
    SYSTEM_STATE_SHUTDOWN       // シャットダウン中
} system_state_t;

extern system_state_t current_system_state;

// 関数プロトタイプ
/**
 * @brief システム初期化
 * @return ESP_OK 成功時、ESP_FAIL 失敗時
 */
esp_err_t system_init(void);

/**
 * @brief ハードウェア初期化
 * @return ESP_OK 成功時、ESP_FAIL 失敗時
 */
esp_err_t hardware_init(void);

/**
 * @brief タスク作成・開始
 * @return ESP_OK 成功時、ESP_FAIL 失敗時
 */
esp_err_t create_tasks(void);

/**
 * @brief システム状態変更
 * @param new_state 新しいシステム状態
 */
void set_system_state(system_state_t new_state);

/**
 * @brief システム状態取得
 * @return 現在のシステム状態
 */
system_state_t get_system_state(void);

/**
 * @brief システムリセット
 */
void system_restart(void);

/**
 * @brief 緊急停止
 */
void emergency_stop(void);

#ifdef __cplusplus
}
#endif

#endif // APP_MAIN_H