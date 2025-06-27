/*
 * Timer HAL Class Implementation
 * 
 * タイマー用のハードウェア抽象化レイヤー実装
 * 
 * 作成者: Kouhei Ito
 * ライセンス: MIT License
 * 
 * Copyright (c) 2025 Kouhei Ito
 */

#include "timer_hal.hpp"
#include "esp_log.h"

namespace hal {

TimerHal::TimerHal(TimerType type) 
    : HalBase("TIMER_HAL")
    , timer_type_(type)
    , esp_timer_handle_(nullptr)
    , gp_timer_handle_(nullptr)
    , active_(false) {
    
    // 設定の初期化
    hr_config_.period_us = 1000;
    hr_config_.auto_reload = true;
    hr_config_.name = "hal_timer";
    
    gp_config_.resolution_hz = 1000000; // 1MHz
    gp_config_.direction = GPTIMER_COUNT_UP;
    gp_config_.alarm_count = 1000;
    gp_config_.auto_reload_on_alarm = true;
    gp_config_.flags = 0;
    
    logDebug("Timer HALクラス作成 タイプ:%s", 
             type == TimerType::HIGH_RESOLUTION ? "高分解能" : "汎用");
}

TimerHal::~TimerHal() {
    if (esp_timer_handle_) {
        esp_timer_stop(esp_timer_handle_);
        esp_timer_delete(esp_timer_handle_);
        logDebug("高分解能タイマー削除");
    }
    
    if (gp_timer_handle_) {
        gptimer_stop(gp_timer_handle_);
        gptimer_del_timer(gp_timer_handle_);
        logDebug("汎用タイマー削除");
    }
    
    logDebug("Timer HALクラス破棄");
}

esp_err_t TimerHal::initialize() {
    setState(State::INITIALIZING);
    setState(State::INITIALIZED);
    logInfo("Timer HAL初期化完了 タイプ:%s", 
            timer_type_ == TimerType::HIGH_RESOLUTION ? "高分解能" : "汎用");
    return ESP_OK;
}

esp_err_t TimerHal::configure() {
    if (!isInitialized()) {
        logError("Timer HALが初期化されていません");
        return ESP_ERR_INVALID_STATE;
    }
    
    logInfo("Timer HAL設定完了");
    return ESP_OK;
}

esp_err_t TimerHal::start() {
    if (!isInitialized()) {
        return ESP_ERR_INVALID_STATE;
    }
    
    setState(State::RUNNING);
    logInfo("Timer HAL開始");
    return ESP_OK;
}

esp_err_t TimerHal::stop() {
    if (esp_timer_handle_) {
        esp_timer_stop(esp_timer_handle_);
    }
    
    if (gp_timer_handle_) {
        gptimer_stop(gp_timer_handle_);
    }
    
    active_ = false;
    setState(State::SUSPENDED);
    logInfo("Timer HAL停止");
    return ESP_OK;
}

esp_err_t TimerHal::reset() {
    if (esp_timer_handle_) {
        esp_timer_stop(esp_timer_handle_);
    }
    
    if (gp_timer_handle_) {
        gptimer_stop(gp_timer_handle_);
        gptimer_set_raw_count(gp_timer_handle_, 0);
    }
    
    active_ = false;
    setState(State::INITIALIZED);
    logInfo("Timer HALリセット完了");
    return ESP_OK;
}

esp_err_t TimerHal::configureHighResolution(const HighResConfig& config, TimerCallback callback) {
    if (!isRunning()) {
        logError("Timer HALが動作していません");
        return ESP_ERR_INVALID_STATE;
    }
    
    if (timer_type_ != TimerType::HIGH_RESOLUTION) {
        logError("高分解能タイマーではありません");
        return ESP_ERR_INVALID_STATE;
    }
    
    // 既存のタイマーを削除
    if (esp_timer_handle_) {
        esp_timer_stop(esp_timer_handle_);
        esp_timer_delete(esp_timer_handle_);
        esp_timer_handle_ = nullptr;
    }
    
    // 設定とコールバックを保存
    hr_config_ = config;
    callback_ = callback;
    
    // タイマー設定
    esp_timer_create_args_t timer_args = {};
    timer_args.callback = espTimerCallback;
    timer_args.arg = this;
    timer_args.name = config.name ? config.name : "hal_timer";
    timer_args.skip_unhandled_events = true;
    
    // タイマーを作成
    esp_err_t ret = esp_timer_create(&timer_args, &esp_timer_handle_);
    if (ret != ESP_OK) {
        logError("高分解能タイマー作成失敗: %s", esp_err_to_name(ret));
        setState(State::ERROR);
        return ret;
    }
    
    logInfo("高分解能タイマー設定完了 周期:%lluus 自動リロード:%s", 
            config.period_us, config.auto_reload ? "有効" : "無効");
    
    return ESP_OK;
}

esp_err_t TimerHal::configureGeneralPurpose(const GeneralPurposeConfig& config, TimerCallback callback) {
    if (!isRunning()) {
        logError("Timer HALが動作していません");
        return ESP_ERR_INVALID_STATE;
    }
    
    if (timer_type_ != TimerType::GENERAL_PURPOSE) {
        logError("汎用タイマーではありません");
        return ESP_ERR_INVALID_STATE;
    }
    
    // 既存のタイマーを削除
    if (gp_timer_handle_) {
        gptimer_stop(gp_timer_handle_);
        gptimer_del_timer(gp_timer_handle_);
        gp_timer_handle_ = nullptr;
    }
    
    // 設定とコールバックを保存
    gp_config_ = config;
    callback_ = callback;
    
    // タイマー設定
    gptimer_config_t timer_config = {};
    timer_config.clk_src = GPTIMER_CLK_SRC_DEFAULT;
    timer_config.direction = config.direction;
    timer_config.resolution_hz = config.resolution_hz;
    timer_config.flags.intr_shared = false;
    
    // タイマーを作成
    esp_err_t ret = gptimer_new_timer(&timer_config, &gp_timer_handle_);
    if (ret != ESP_OK) {
        logError("汎用タイマー作成失敗: %s", esp_err_to_name(ret));
        setState(State::ERROR);
        return ret;
    }
    
    // アラーム設定
    gptimer_alarm_config_t alarm_config = {};
    alarm_config.alarm_count = config.alarm_count;
    alarm_config.reload_count = 0;
    alarm_config.flags.auto_reload_on_alarm = config.auto_reload_on_alarm;
    
    ret = gptimer_set_alarm_action(gp_timer_handle_, &alarm_config);
    if (ret != ESP_OK) {
        logError("アラーム設定失敗: %s", esp_err_to_name(ret));
        gptimer_del_timer(gp_timer_handle_);
        gp_timer_handle_ = nullptr;
        return ret;
    }
    
    // イベントコールバック設定
    gptimer_event_callbacks_t cbs = {};
    cbs.on_alarm = gpTimerCallback;
    
    ret = gptimer_register_event_callbacks(gp_timer_handle_, &cbs, this);
    if (ret != ESP_OK) {
        logError("コールバック設定失敗: %s", esp_err_to_name(ret));
        gptimer_del_timer(gp_timer_handle_);
        gp_timer_handle_ = nullptr;
        return ret;
    }
    
    // タイマーを有効化
    ret = gptimer_enable(gp_timer_handle_);
    if (ret != ESP_OK) {
        logError("タイマー有効化失敗: %s", esp_err_to_name(ret));
        gptimer_del_timer(gp_timer_handle_);
        gp_timer_handle_ = nullptr;
        return ret;
    }
    
    logInfo("汎用タイマー設定完了 分解能:%dHz アラーム:%llu 自動リロード:%s", 
            config.resolution_hz, config.alarm_count, 
            config.auto_reload_on_alarm ? "有効" : "無効");
    
    return ESP_OK;
}

esp_err_t TimerHal::setPeriod(uint64_t period_us) {
    if (timer_type_ == TimerType::HIGH_RESOLUTION) {
        hr_config_.period_us = period_us;
        
        if (esp_timer_handle_ && active_) {
            // タイマーを停止して再開
            esp_timer_stop(esp_timer_handle_);
            
            esp_err_t ret;
            if (hr_config_.auto_reload) {
                ret = esp_timer_start_periodic(esp_timer_handle_, period_us);
            } else {
                ret = esp_timer_start_once(esp_timer_handle_, period_us);
            }
            
            if (ret != ESP_OK) {
                logError("タイマー周期変更失敗: %s", esp_err_to_name(ret));
                return ret;
            }
        }
        
        logInfo("高分解能タイマー周期変更: %lluus", period_us);
        return ESP_OK;
    } else {
        logError("汎用タイマーでは周期設定はサポートされていません");
        return ESP_ERR_NOT_SUPPORTED;
    }
}

esp_err_t TimerHal::setAlarmValue(uint64_t alarm_count) {
    if (timer_type_ != TimerType::GENERAL_PURPOSE || !gp_timer_handle_) {
        logError("汎用タイマーが設定されていません");
        return ESP_ERR_INVALID_STATE;
    }
    
    gp_config_.alarm_count = alarm_count;
    
    gptimer_alarm_config_t alarm_config = {};
    alarm_config.alarm_count = alarm_count;
    alarm_config.reload_count = 0;
    alarm_config.flags.auto_reload_on_alarm = gp_config_.auto_reload_on_alarm;
    
    esp_err_t ret = gptimer_set_alarm_action(gp_timer_handle_, &alarm_config);
    if (ret != ESP_OK) {
        logError("アラーム値設定失敗: %s", esp_err_to_name(ret));
        return ret;
    }
    
    logInfo("アラーム値設定: %llu", alarm_count);
    return ESP_OK;
}

esp_err_t TimerHal::getCurrentCount(uint64_t& count_value) {
    if (timer_type_ != TimerType::GENERAL_PURPOSE || !gp_timer_handle_) {
        logError("汎用タイマーが設定されていません");
        return ESP_ERR_INVALID_STATE;
    }
    
    esp_err_t ret = gptimer_get_raw_count(gp_timer_handle_, &count_value);
    if (ret != ESP_OK) {
        logError("カウント値取得失敗: %s", esp_err_to_name(ret));
        return ret;
    }
    
    return ESP_OK;
}

esp_err_t TimerHal::setCurrentCount(uint64_t count_value) {
    if (timer_type_ != TimerType::GENERAL_PURPOSE || !gp_timer_handle_) {
        logError("汎用タイマーが設定されていません");
        return ESP_ERR_INVALID_STATE;
    }
    
    esp_err_t ret = gptimer_set_raw_count(gp_timer_handle_, count_value);
    if (ret != ESP_OK) {
        logError("カウント値設定失敗: %s", esp_err_to_name(ret));
        return ret;
    }
    
    logDebug("カウント値設定: %llu", count_value);
    return ESP_OK;
}

int64_t TimerHal::getCurrentTime() {
    return esp_timer_get_time();
}

void TimerHal::delay(uint32_t delay_us) {
    esp_rom_delay_us(delay_us);
}

esp_err_t TimerHal::startOneShot(uint64_t timeout_us, TimerCallback callback, const char* name) {
    if (timer_type_ != TimerType::HIGH_RESOLUTION) {
        logError("ワンショットタイマーは高分解能タイマーでのみサポートされています");
        return ESP_ERR_NOT_SUPPORTED;
    }
    
    if (!esp_timer_handle_) {
        // 一時的な設定でタイマーを作成
        HighResConfig temp_config;
        temp_config.period_us = timeout_us;
        temp_config.auto_reload = false;
        temp_config.name = name ? name : "oneshot_timer";
        
        esp_err_t ret = configureHighResolution(temp_config, callback);
        if (ret != ESP_OK) {
            return ret;
        }
    }
    
    callback_ = callback;
    
    esp_err_t ret = esp_timer_start_once(esp_timer_handle_, timeout_us);
    if (ret != ESP_OK) {
        logError("ワンショットタイマー開始失敗: %s", esp_err_to_name(ret));
        return ret;
    }
    
    active_ = true;
    logInfo("ワンショットタイマー開始: %lluus", timeout_us);
    
    return ESP_OK;
}

bool TimerHal::isActive() const {
    if (timer_type_ == TimerType::HIGH_RESOLUTION) {
        return esp_timer_handle_ && esp_timer_is_active(esp_timer_handle_);
    } else {
        return active_;
    }
}

void TimerHal::espTimerCallback(void* arg) {
    TimerHal* instance = static_cast<TimerHal*>(arg);
    if (instance && instance->callback_) {
        instance->callback_();
    }
}

bool IRAM_ATTR TimerHal::gpTimerCallback(gptimer_handle_t timer, 
                                        const gptimer_alarm_event_data_t* edata, 
                                        void* user_data) {
    TimerHal* instance = static_cast<TimerHal*>(user_data);
    if (instance && instance->callback_) {
        return instance->callback_();
    }
    return false;
}

} // namespace hal