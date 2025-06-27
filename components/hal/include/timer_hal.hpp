/*
 * Timer HAL Class
 * 
 * タイマー用のハードウェア抽象化レイヤー
 * ESP-IDF Timer APIをC++クラスでラップ
 * 
 * 作成者: Kouhei Ito
 * ライセンス: MIT License
 * 
 * Copyright (c) 2025 Kouhei Ito
 */

#ifndef TIMER_HAL_HPP
#define TIMER_HAL_HPP

#include "hal_base.hpp"
#include "driver/gptimer.h"
#include "esp_timer.h"
#include <functional>
#include <memory>

namespace hal {

/**
 * @brief Timer HALクラス
 * 
 * タイマー操作の抽象化レイヤー
 * ESP-IDF Timer APIのC++ラッパー
 */
class TimerHal : public HalBase {
public:
    /**
     * @brief タイマータイプ列挙型
     */
    enum class TimerType {
        HIGH_RESOLUTION,    // 高分解能タイマー (esp_timer)
        GENERAL_PURPOSE     // 汎用タイマー (gptimer)
    };

    /**
     * @brief 高分解能タイマー設定構造体
     */
    struct HighResConfig {
        uint64_t period_us;         // 周期（マイクロ秒）
        bool auto_reload;           // 自動リロード
        const char* name;           // タイマー名
    };

    /**
     * @brief 汎用タイマー設定構造体
     */
    struct GeneralPurposeConfig {
        uint32_t resolution_hz;     // 分解能（Hz）
        gptimer_count_direction_t direction;  // カウント方向
        uint64_t alarm_count;       // アラーム値
        bool auto_reload_on_alarm;  // アラーム時自動リロード
        uint32_t flags;             // 追加フラグ
    };

    /**
     * @brief タイマーコールバック関数型
     */
    using TimerCallback = std::function<bool(void)>;  // 戻り値: 高優先度タスクを起動するかどうか

public:
    /**
     * @brief コンストラクタ
     * @param type タイマータイプ
     */
    explicit TimerHal(TimerType type = TimerType::HIGH_RESOLUTION);

    /**
     * @brief デストラクタ
     */
    virtual ~TimerHal();

    /**
     * @brief 初期化
     * @return esp_err_t 初期化結果
     */
    esp_err_t initialize() override;

    /**
     * @brief 設定変更
     * @return esp_err_t 設定結果
     */
    esp_err_t configure() override;

    /**
     * @brief 開始
     * @return esp_err_t 開始結果
     */
    esp_err_t start() override;

    /**
     * @brief 停止
     * @return esp_err_t 停止結果
     */
    esp_err_t stop() override;

    /**
     * @brief リセット
     * @return esp_err_t リセット結果
     */
    esp_err_t reset() override;

    /**
     * @brief 高分解能タイマー設定
     * @param config 高分解能タイマー設定
     * @param callback コールバック関数
     * @return esp_err_t 設定結果
     */
    esp_err_t configureHighResolution(const HighResConfig& config, TimerCallback callback);

    /**
     * @brief 汎用タイマー設定
     * @param config 汎用タイマー設定
     * @param callback コールバック関数
     * @return esp_err_t 設定結果
     */
    esp_err_t configureGeneralPurpose(const GeneralPurposeConfig& config, TimerCallback callback);

    /**
     * @brief タイマー周期変更
     * @param period_us 新しい周期（マイクロ秒）
     * @return esp_err_t 変更結果
     */
    esp_err_t setPeriod(uint64_t period_us);

    /**
     * @brief アラーム値設定（汎用タイマーのみ）
     * @param alarm_count アラーム値
     * @return esp_err_t 設定結果
     */
    esp_err_t setAlarmValue(uint64_t alarm_count);

    /**
     * @brief 現在のカウント値取得（汎用タイマーのみ）
     * @param count_value カウント値格納先
     * @return esp_err_t 取得結果
     */
    esp_err_t getCurrentCount(uint64_t& count_value);

    /**
     * @brief カウント値設定（汎用タイマーのみ）
     * @param count_value 設定するカウント値
     * @return esp_err_t 設定結果
     */
    esp_err_t setCurrentCount(uint64_t count_value);

    /**
     * @brief 現在時刻取得（高分解能タイマーのみ）
     * @return int64_t 現在時刻（マイクロ秒）
     */
    int64_t getCurrentTime();

    /**
     * @brief 遅延実行（高分解能タイマーのみ）
     * @param delay_us 遅延時間（マイクロ秒）
     */
    void delay(uint32_t delay_us);

    /**
     * @brief ワンショットタイマー実行
     * @param timeout_us タイムアウト時間（マイクロ秒）
     * @param callback コールバック関数
     * @param name タイマー名
     * @return esp_err_t 実行結果
     */
    esp_err_t startOneShot(uint64_t timeout_us, TimerCallback callback, const char* name = nullptr);

    /**
     * @brief タイマータイプ取得
     * @return TimerType タイマータイプ
     */
    TimerType getTimerType() const { return timer_type_; }

    /**
     * @brief 動作中確認
     * @return bool 動作中の場合true
     */
    bool isActive() const;

private:
    TimerType timer_type_;              // タイマータイプ
    
    // 高分解能タイマー用
    esp_timer_handle_t esp_timer_handle_;
    HighResConfig hr_config_;
    
    // 汎用タイマー用
    gptimer_handle_t gp_timer_handle_;
    GeneralPurposeConfig gp_config_;
    
    TimerCallback callback_;            // コールバック関数
    bool active_;                       // アクティブ状態

    /**
     * @brief 高分解能タイマーコールバック
     * @param arg 引数（TimerHalインスタンス）
     */
    static void espTimerCallback(void* arg);

    /**
     * @brief 汎用タイマーコールバック
     * @param timer タイマーハンドル
     * @param edata イベントデータ
     * @param user_data ユーザーデータ（TimerHalインスタンス）
     * @return bool 高優先度タスクを起動するかどうか
     */
    static bool IRAM_ATTR gpTimerCallback(gptimer_handle_t timer, 
                                         const gptimer_alarm_event_data_t* edata, 
                                         void* user_data);
};

} // namespace hal

#endif // TIMER_HAL_HPP