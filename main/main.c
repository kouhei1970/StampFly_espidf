/*
 * StampFly ESP-IDF版 基本テスト用メインアプリケーション
 * 
 * ESP32-S3ベースドローン制御システム
 * 
 * 作成者: Kouhei Ito
 * ライセンス: MIT License
 * 
 * Copyright (c) 2025 Kouhei Ito
 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void app_main(void)
{
    // 極最小限のテスト - HAL問題回避のため印刷機能なし
    int counter = 0;
    while(1) {
        counter++;
        vTaskDelay(1000 / portTICK_PERIOD_MS); // 1秒間隔
        
        // カウンタが1000に達したらリセット（メモリリーク防止）
        if (counter >= 1000) {
            counter = 0;
        }
    }
}