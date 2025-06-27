/*
 * PWM HAL Class Implementation
 * 
 * PWM出力用のハードウェア抽象化レイヤー実装
 * 
 * 作成者: Kouhei Ito
 * ライセンス: MIT License
 * 
 * Copyright (c) 2025 Kouhei Ito
 */

#include "pwm_hal.hpp"
#include "esp_log.h"
#include <algorithm>

namespace hal {

PwmHal::PwmHal() 
    : HalBase("PWM_HAL")
    , fade_service_installed_(false) {
    logDebug("PWM HALクラス作成");
}

PwmHal::~PwmHal() {
    if (fade_service_installed_) {
        ledc_fade_service_uninstall();
        logDebug("LEDCフェードサービス削除");
    }
    
    logDebug("PWM HALクラス破棄");
}

esp_err_t PwmHal::initialize() {
    setState(State::INITIALIZING);
    setState(State::INITIALIZED);
    logInfo("PWM HAL初期化完了");
    return ESP_OK;
}

esp_err_t PwmHal::configure() {
    if (!isInitialized()) {
        logError("PWM HALが初期化されていません");
        return ESP_ERR_INVALID_STATE;
    }
    
    // 既存のタイマーとチャンネルを再設定
    for (const auto& pair : timer_configs_) {
        esp_err_t ret = configureTimer(pair.second);
        if (ret != ESP_OK) {
            logError("タイマー%d再設定失敗: %s", pair.first, esp_err_to_name(ret));
            return ret;
        }
    }
    
    for (const auto& pair : channel_configs_) {
        esp_err_t ret = configureChannel(pair.second);
        if (ret != ESP_OK) {
            logError("チャンネル%d再設定失敗: %s", pair.first, esp_err_to_name(ret));
            return ret;
        }
    }
    
    logInfo("PWM HAL設定完了");
    return ESP_OK;
}

esp_err_t PwmHal::start() {
    if (!isInitialized()) {
        return ESP_ERR_INVALID_STATE;
    }
    
    setState(State::RUNNING);
    logInfo("PWM HAL開始");
    return ESP_OK;
}

esp_err_t PwmHal::stop() {
    // 全チャンネルの出力を停止
    for (const auto& pair : channel_configs_) {
        stopOutput(pair.first, pair.second.speed_mode, 0);
    }
    
    setState(State::SUSPENDED);
    logInfo("PWM HAL停止");
    return ESP_OK;
}

esp_err_t PwmHal::reset() {
    // 全設定をクリア
    timer_configs_.clear();
    channel_configs_.clear();
    
    setState(State::INITIALIZED);
    logInfo("PWM HALリセット完了");
    return ESP_OK;
}

esp_err_t PwmHal::configureTimer(const TimerConfig& config) {
    if (!isInitialized()) {
        logError("PWM HALが初期化されていません");
        return ESP_ERR_INVALID_STATE;
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    // LEDCタイマー設定
    ledc_timer_config_t timer_conf = {};
    timer_conf.speed_mode = static_cast<ledc_mode_t>(config.speed_mode);
    timer_conf.timer_num = config.timer_num;
    timer_conf.duty_resolution = static_cast<ledc_timer_bit_t>(config.resolution);
    timer_conf.freq_hz = config.frequency;
    timer_conf.clk_cfg = config.clk_cfg;
    
    esp_err_t ret = ledc_timer_config(&timer_conf);
    if (ret != ESP_OK) {
        logError("PWMタイマー設定失敗 タイマー:%d: %s", 
                 config.timer_num, esp_err_to_name(ret));
        return ret;
    }
    
    // 設定を保存
    timer_configs_[config.timer_num] = config;
    
    logInfo("PWMタイマー設定完了 タイマー:%d 周波数:%dHz 分解能:%dビット",
            config.timer_num, config.frequency, 
            static_cast<int>(config.resolution) + 1);
    
    return ESP_OK;
}

esp_err_t PwmHal::configureChannel(const ChannelConfig& config) {
    if (!isInitialized()) {
        logError("PWM HALが初期化されていません");
        return ESP_ERR_INVALID_STATE;
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    // LEDCチャンネル設定
    ledc_channel_config_t ledc_channel = {};
    ledc_channel.channel = config.channel;
    ledc_channel.duty = config.duty;
    ledc_channel.gpio_num = config.gpio_num;
    ledc_channel.speed_mode = static_cast<ledc_mode_t>(config.speed_mode);
    ledc_channel.hpoint = config.hpoint;
    ledc_channel.timer_sel = config.timer_sel;
    ledc_channel.flags.output_invert = 0;
    
    esp_err_t ret = ledc_channel_config(&ledc_channel);
    if (ret != ESP_OK) {
        logError("PWMチャンネル設定失敗 チャンネル:%d: %s", 
                 config.channel, esp_err_to_name(ret));
        return ret;
    }
    
    // 設定を保存
    channel_configs_[config.channel] = config;
    
    logInfo("PWMチャンネル設定完了 チャンネル:%d GPIO:%d タイマー:%d デューティ:%d",
            config.channel, config.gpio_num, config.timer_sel, config.duty);
    
    return ESP_OK;
}

esp_err_t PwmHal::setDuty(ledc_channel_t channel, SpeedMode speed_mode, uint32_t duty) {
    if (!isRunning()) {
        logError("PWM HALが動作していません");
        return ESP_ERR_INVALID_STATE;
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    // デューティ比設定
    esp_err_t ret = ledc_set_duty(static_cast<ledc_mode_t>(speed_mode), channel, duty);
    if (ret != ESP_OK) {
        logError("デューティ比設定失敗 チャンネル:%d: %s", channel, esp_err_to_name(ret));
        return ret;
    }
    
    // 設定を更新
    ret = ledc_update_duty(static_cast<ledc_mode_t>(speed_mode), channel);
    if (ret != ESP_OK) {
        logError("デューティ比更新失敗 チャンネル:%d: %s", channel, esp_err_to_name(ret));
        return ret;
    }
    
    // 保存された設定を更新
    auto it = channel_configs_.find(channel);
    if (it != channel_configs_.end()) {
        it->second.duty = duty;
    }
    
    logDebug("デューティ比設定完了 チャンネル:%d デューティ:%d", channel, duty);
    return ESP_OK;
}

esp_err_t PwmHal::getDuty(ledc_channel_t channel, SpeedMode speed_mode, uint32_t& duty) {
    if (!isRunning()) {
        logError("PWM HALが動作していません");
        return ESP_ERR_INVALID_STATE;
    }
    
    duty = ledc_get_duty(static_cast<ledc_mode_t>(speed_mode), channel);
    return ESP_OK;
}

esp_err_t PwmHal::setFrequency(ledc_timer_t timer_num, SpeedMode speed_mode, uint32_t frequency) {
    if (!isRunning()) {
        logError("PWM HALが動作していません");
        return ESP_ERR_INVALID_STATE;
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    esp_err_t ret = ledc_set_freq(static_cast<ledc_mode_t>(speed_mode), timer_num, frequency);
    if (ret != ESP_OK) {
        logError("周波数設定失敗 タイマー:%d: %s", timer_num, esp_err_to_name(ret));
        return ret;
    }
    
    // 保存された設定を更新
    auto it = timer_configs_.find(timer_num);
    if (it != timer_configs_.end()) {
        it->second.frequency = frequency;
    }
    
    logInfo("周波数設定完了 タイマー:%d 周波数:%dHz", timer_num, frequency);
    return ESP_OK;
}

esp_err_t PwmHal::getFrequency(ledc_timer_t timer_num, SpeedMode speed_mode, uint32_t& frequency) {
    if (!isRunning()) {
        logError("PWM HALが動作していません");
        return ESP_ERR_INVALID_STATE;
    }
    
    frequency = ledc_get_freq(static_cast<ledc_mode_t>(speed_mode), timer_num);
    return ESP_OK;
}

esp_err_t PwmHal::setDutyPercentage(ledc_channel_t channel, SpeedMode speed_mode, float percentage) {
    // チャンネル設定から分解能を取得
    auto it = channel_configs_.find(channel);
    if (it == channel_configs_.end()) {
        logError("未設定のチャンネル: %d", channel);
        return ESP_ERR_INVALID_ARG;
    }
    
    // タイマー設定から分解能を取得
    auto timer_it = timer_configs_.find(it->second.timer_sel);
    if (timer_it == timer_configs_.end()) {
        logError("未設定のタイマー: %d", it->second.timer_sel);
        return ESP_ERR_INVALID_ARG;
    }
    
    // パーセンテージ範囲確認
    if (percentage < 0.0f || percentage > 100.0f) {
        logError("無効なパーセンテージ: %f", percentage);
        return ESP_ERR_INVALID_ARG;
    }
    
    // デューティ値に変換
    uint32_t duty = percentageToDuty(percentage, timer_it->second.resolution);
    
    return setDuty(channel, speed_mode, duty);
}

esp_err_t PwmHal::getDutyPercentage(ledc_channel_t channel, SpeedMode speed_mode, float& percentage) {
    uint32_t duty;
    esp_err_t ret = getDuty(channel, speed_mode, duty);
    if (ret != ESP_OK) {
        return ret;
    }
    
    // チャンネル設定から分解能を取得
    auto it = channel_configs_.find(channel);
    if (it == channel_configs_.end()) {
        logError("未設定のチャンネル: %d", channel);
        return ESP_ERR_INVALID_ARG;
    }
    
    // タイマー設定から分解能を取得
    auto timer_it = timer_configs_.find(it->second.timer_sel);
    if (timer_it == timer_configs_.end()) {
        logError("未設定のタイマー: %d", it->second.timer_sel);
        return ESP_ERR_INVALID_ARG;
    }
    
    // パーセンテージに変換
    percentage = dutyToPercentage(duty, timer_it->second.resolution);
    
    return ESP_OK;
}

esp_err_t PwmHal::startFade(ledc_channel_t channel, SpeedMode speed_mode, const FadeConfig& fade_config) {
    if (!isRunning()) {
        logError("PWM HALが動作していません");
        return ESP_ERR_INVALID_STATE;
    }
    
    // フェードサービスが初期化されていない場合は初期化
    if (!fade_service_installed_) {
        esp_err_t ret = installFadeService();
        if (ret != ESP_OK) {
            return ret;
        }
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    // フェード設定
    esp_err_t ret = ledc_set_fade_with_time(static_cast<ledc_mode_t>(speed_mode), 
                                           channel, 
                                           fade_config.target_duty, 
                                           fade_config.max_fade_time_ms);
    if (ret != ESP_OK) {
        logError("フェード設定失敗 チャンネル:%d: %s", channel, esp_err_to_name(ret));
        return ret;
    }
    
    // フェード開始
    ret = ledc_fade_start(static_cast<ledc_mode_t>(speed_mode), 
                         channel, 
                         fade_config.fade_mode);
    if (ret != ESP_OK) {
        logError("フェード開始失敗 チャンネル:%d: %s", channel, esp_err_to_name(ret));
        return ret;
    }
    
    logInfo("フェード開始 チャンネル:%d 目標:%d 時間:%dms", 
            channel, fade_config.target_duty, fade_config.max_fade_time_ms);
    
    return ESP_OK;
}

esp_err_t PwmHal::stopFade(ledc_channel_t channel, SpeedMode speed_mode) {
    if (!isRunning()) {
        logError("PWM HALが動作していません");
        return ESP_ERR_INVALID_STATE;
    }
    
    esp_err_t ret = ledc_fade_stop(static_cast<ledc_mode_t>(speed_mode), channel);
    if (ret != ESP_OK) {
        logError("フェード停止失敗 チャンネル:%d: %s", channel, esp_err_to_name(ret));
        return ret;
    }
    
    logInfo("フェード停止 チャンネル:%d", channel);
    return ESP_OK;
}

esp_err_t PwmHal::stopOutput(ledc_channel_t channel, SpeedMode speed_mode, uint32_t idle_level) {
    if (!isRunning()) {
        logError("PWM HALが動作していません");
        return ESP_ERR_INVALID_STATE;
    }
    
    esp_err_t ret = ledc_stop(static_cast<ledc_mode_t>(speed_mode), channel, idle_level);
    if (ret != ESP_OK) {
        logError("PWM出力停止失敗 チャンネル:%d: %s", channel, esp_err_to_name(ret));
        return ret;
    }
    
    logInfo("PWM出力停止 チャンネル:%d アイドルレベル:%d", channel, idle_level);
    return ESP_OK;
}

esp_err_t PwmHal::resumeOutput(ledc_channel_t channel, SpeedMode speed_mode) {
    if (!isRunning()) {
        logError("PWM HALが動作していません");
        return ESP_ERR_INVALID_STATE;
    }
    
    // 保存された設定でPWM出力を再開
    auto it = channel_configs_.find(channel);
    if (it == channel_configs_.end()) {
        logError("未設定のチャンネル: %d", channel);
        return ESP_ERR_INVALID_ARG;
    }
    
    esp_err_t ret = setDuty(channel, speed_mode, it->second.duty);
    if (ret != ESP_OK) {
        return ret;
    }
    
    logInfo("PWM出力再開 チャンネル:%d", channel);
    return ESP_OK;
}

uint32_t PwmHal::getMaxDuty(Resolution resolution) {
    return (1U << (static_cast<int>(resolution) + 1)) - 1;
}

uint32_t PwmHal::percentageToDuty(float percentage, Resolution resolution) {
    if (percentage < 0.0f) percentage = 0.0f;
    if (percentage > 100.0f) percentage = 100.0f;
    
    uint32_t max_duty = getMaxDuty(resolution);
    return static_cast<uint32_t>((percentage / 100.0f) * max_duty);
}

float PwmHal::dutyToPercentage(uint32_t duty, Resolution resolution) {
    uint32_t max_duty = getMaxDuty(resolution);
    if (max_duty == 0) return 0.0f;
    
    return (static_cast<float>(duty) / max_duty) * 100.0f;
}

esp_err_t PwmHal::installFadeService() {
    if (fade_service_installed_) {
        return ESP_OK;
    }
    
    esp_err_t ret = ledc_fade_service_install(0);
    if (ret != ESP_OK) {
        logError("フェードサービス初期化失敗: %s", esp_err_to_name(ret));
        return ret;
    }
    
    fade_service_installed_ = true;
    logInfo("LEDCフェードサービス初期化完了");
    
    return ESP_OK;
}

} // namespace hal