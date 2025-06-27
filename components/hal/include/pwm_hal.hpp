/*
 * PWM HAL Class
 * 
 * PWM出力用のハードウェア抽象化レイヤー
 * ESP-IDF LEDC APIをC++クラスでラップ
 * 
 * 作成者: Kouhei Ito
 * ライセンス: MIT License
 * 
 * Copyright (c) 2025 Kouhei Ito
 */

#ifndef PWM_HAL_HPP
#define PWM_HAL_HPP

#include "hal_base.hpp"
#include "driver/ledc.h"
#include <map>
#include <memory>

namespace hal {

/**
 * @brief PWM HALクラス
 * 
 * PWM出力の抽象化レイヤー
 * ESP-IDF LEDC APIのC++ラッパー
 */
class PwmHal : public HalBase {
public:
    /**
     * @brief PWMスピードモード列挙型
     */
    enum class SpeedMode {
        LOW_SPEED = LEDC_LOW_SPEED_MODE,
        HIGH_SPEED = LEDC_HIGH_SPEED_MODE
    };

    /**
     * @brief PWMタイマー分解能列挙型
     */
    enum class Resolution {
        BITS_1 = LEDC_TIMER_1_BIT,
        BITS_2 = LEDC_TIMER_2_BIT,
        BITS_3 = LEDC_TIMER_3_BIT,
        BITS_4 = LEDC_TIMER_4_BIT,
        BITS_5 = LEDC_TIMER_5_BIT,
        BITS_6 = LEDC_TIMER_6_BIT,
        BITS_7 = LEDC_TIMER_7_BIT,
        BITS_8 = LEDC_TIMER_8_BIT,
        BITS_9 = LEDC_TIMER_9_BIT,
        BITS_10 = LEDC_TIMER_10_BIT,
        BITS_11 = LEDC_TIMER_11_BIT,
        BITS_12 = LEDC_TIMER_12_BIT,
        BITS_13 = LEDC_TIMER_13_BIT,
        BITS_14 = LEDC_TIMER_14_BIT,
        BITS_15 = LEDC_TIMER_15_BIT,
        BITS_16 = LEDC_TIMER_16_BIT,
        BITS_17 = LEDC_TIMER_17_BIT,
        BITS_18 = LEDC_TIMER_18_BIT,
        BITS_19 = LEDC_TIMER_19_BIT,
        BITS_20 = LEDC_TIMER_20_BIT
    };

    /**
     * @brief PWMタイマー設定構造体
     */
    struct TimerConfig {
        ledc_timer_t timer_num;     // タイマー番号
        SpeedMode speed_mode;       // スピードモード
        Resolution resolution;      // 分解能
        uint32_t frequency;         // 周波数（Hz）
        ledc_clk_cfg_t clk_cfg;    // クロック設定
    };

    /**
     * @brief PWMチャンネル設定構造体
     */
    struct ChannelConfig {
        ledc_channel_t channel;     // チャンネル番号
        ledc_timer_t timer_sel;     // 使用するタイマー
        SpeedMode speed_mode;       // スピードモード
        gpio_num_t gpio_num;        // GPIOピン番号
        uint32_t duty;              // 初期デューティ比
        int hpoint;                 // フェーズ遅延
    };

    /**
     * @brief PWMフェードタスク設定構造体
     */
    struct FadeConfig {
        uint32_t target_duty;       // 目標デューティ比
        int max_fade_time_ms;       // 最大フェード時間（ミリ秒）
        ledc_fade_mode_t fade_mode; // フェードモード
    };

public:
    /**
     * @brief コンストラクタ
     */
    PwmHal();

    /**
     * @brief デストラクタ
     */
    virtual ~PwmHal();

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
     * @brief PWMタイマー設定
     * @param config タイマー設定
     * @return esp_err_t 設定結果
     */
    esp_err_t configureTimer(const TimerConfig& config);

    /**
     * @brief PWMチャンネル設定
     * @param config チャンネル設定
     * @return esp_err_t 設定結果
     */
    esp_err_t configureChannel(const ChannelConfig& config);

    /**
     * @brief デューティ比設定
     * @param channel チャンネル番号
     * @param speed_mode スピードモード
     * @param duty デューティ比
     * @return esp_err_t 設定結果
     */
    esp_err_t setDuty(ledc_channel_t channel, SpeedMode speed_mode, uint32_t duty);

    /**
     * @brief デューティ比取得
     * @param channel チャンネル番号
     * @param speed_mode スピードモード
     * @param duty デューティ比格納先
     * @return esp_err_t 取得結果
     */
    esp_err_t getDuty(ledc_channel_t channel, SpeedMode speed_mode, uint32_t& duty);

    /**
     * @brief PWM周波数設定
     * @param timer_num タイマー番号
     * @param speed_mode スピードモード
     * @param frequency 周波数（Hz）
     * @return esp_err_t 設定結果
     */
    esp_err_t setFrequency(ledc_timer_t timer_num, SpeedMode speed_mode, uint32_t frequency);

    /**
     * @brief PWM周波数取得
     * @param timer_num タイマー番号
     * @param speed_mode スピードモード
     * @param frequency 周波数格納先
     * @return esp_err_t 取得結果
     */
    esp_err_t getFrequency(ledc_timer_t timer_num, SpeedMode speed_mode, uint32_t& frequency);

    /**
     * @brief パーセンテージでデューティ比設定
     * @param channel チャンネル番号
     * @param speed_mode スピードモード
     * @param percentage パーセンテージ（0.0-100.0）
     * @return esp_err_t 設定結果
     */
    esp_err_t setDutyPercentage(ledc_channel_t channel, SpeedMode speed_mode, float percentage);

    /**
     * @brief パーセンテージでデューティ比取得
     * @param channel チャンネル番号
     * @param speed_mode スピードモード
     * @param percentage パーセンテージ格納先
     * @return esp_err_t 取得結果
     */
    esp_err_t getDutyPercentage(ledc_channel_t channel, SpeedMode speed_mode, float& percentage);

    /**
     * @brief PWMフェード開始
     * @param channel チャンネル番号
     * @param speed_mode スピードモード
     * @param fade_config フェード設定
     * @return esp_err_t 開始結果
     */
    esp_err_t startFade(ledc_channel_t channel, SpeedMode speed_mode, const FadeConfig& fade_config);

    /**
     * @brief PWMフェード停止
     * @param channel チャンネル番号
     * @param speed_mode スピードモード
     * @return esp_err_t 停止結果
     */
    esp_err_t stopFade(ledc_channel_t channel, SpeedMode speed_mode);

    /**
     * @brief PWM出力停止
     * @param channel チャンネル番号
     * @param speed_mode スピードモード
     * @param idle_level 停止時のレベル
     * @return esp_err_t 停止結果
     */
    esp_err_t stopOutput(ledc_channel_t channel, SpeedMode speed_mode, uint32_t idle_level);

    /**
     * @brief PWM出力再開
     * @param channel チャンネル番号
     * @param speed_mode スピードモード
     * @return esp_err_t 再開結果
     */
    esp_err_t resumeOutput(ledc_channel_t channel, SpeedMode speed_mode);

    /**
     * @brief 最大デューティ値取得
     * @param resolution 分解能
     * @return uint32_t 最大デューティ値
     */
    static uint32_t getMaxDuty(Resolution resolution);

    /**
     * @brief パーセンテージをデューティ値に変換
     * @param percentage パーセンテージ（0.0-100.0）
     * @param resolution 分解能
     * @return uint32_t デューティ値
     */
    static uint32_t percentageToDuty(float percentage, Resolution resolution);

    /**
     * @brief デューティ値をパーセンテージに変換
     * @param duty デューティ値
     * @param resolution 分解能
     * @return float パーセンテージ
     */
    static float dutyToPercentage(uint32_t duty, Resolution resolution);

private:
    std::map<ledc_timer_t, TimerConfig> timer_configs_;     // タイマー設定管理
    std::map<ledc_channel_t, ChannelConfig> channel_configs_; // チャンネル設定管理
    bool fade_service_installed_;                           // フェードサービス初期化フラグ

    /**
     * @brief フェードサービス初期化
     * @return esp_err_t 初期化結果
     */
    esp_err_t installFadeService();
};

} // namespace hal

#endif // PWM_HAL_HPP