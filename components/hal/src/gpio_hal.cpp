/*
 * GPIO HAL Class Implementation
 * 
 * GPIO操作用のハードウェア抽象化レイヤー実装
 * ESP-IDF GPIO APIをC++クラスでラップ
 * 
 * 作成者: Kouhei Ito
 * ライセンス: MIT License
 * 
 * Copyright (c) 2025 Kouhei Ito
 */

#include "gpio_hal.hpp"
#include "driver/gpio.h"
#include "esp_log.h"

namespace hal {

// 静的メンバ変数の初期化
std::map<gpio_num_t, GpioHal*> GpioHal::pin_map_;
bool GpioHal::isr_service_installed_ = false;

GpioHal::GpioHal() : HalBase("GPIO_HAL") {
    logDebug("GPIO HALクラス作成");
}

GpioHal::~GpioHal() {
    // 管理中のピンの割り込みを無効化
    for (const auto& pair : pin_configs_) {
        disableInterrupt(pair.first);
    }
    
    // ピンマップから自身を削除
    for (auto it = pin_map_.begin(); it != pin_map_.end();) {
        if (it->second == this) {
            it = pin_map_.erase(it);
        } else {
            ++it;
        }
    }
    
    logDebug("GPIO HALクラス破棄");
}

esp_err_t GpioHal::initialize() {
    setState(State::INITIALIZING);
    
    // ISRサービスの初期化
    esp_err_t ret = installIsrService();
    if (ret != ESP_OK) {
        logError("ISRサービス初期化失敗: %s", esp_err_to_name(ret));
        setState(State::ERROR);
        return ret;
    }
    
    setState(State::INITIALIZED);
    logInfo("GPIO HAL初期化完了");
    return ESP_OK;
}

esp_err_t GpioHal::configure() {
    if (!isInitialized()) {
        logError("GPIO HALが初期化されていません");
        return ESP_ERR_INVALID_STATE;
    }
    
    // 設定済みのピンを再設定
    for (const auto& pair : pin_configs_) {
        esp_err_t ret = configurePin(pair.second);
        if (ret != ESP_OK) {
            logError("ピン%d設定失敗: %s", pair.first, esp_err_to_name(ret));
            return ret;
        }
    }
    
    logInfo("GPIO HAL設定完了");
    return ESP_OK;
}

esp_err_t GpioHal::start() {
    if (!isInitialized()) {
        return ESP_ERR_INVALID_STATE;
    }
    
    setState(State::RUNNING);
    logInfo("GPIO HAL開始");
    return ESP_OK;
}

esp_err_t GpioHal::stop() {
    setState(State::SUSPENDED);
    
    // 全ての割り込みを無効化
    for (const auto& pair : pin_configs_) {
        disableInterrupt(pair.first);
    }
    
    logInfo("GPIO HAL停止");
    return ESP_OK;
}

esp_err_t GpioHal::reset() {
    // 全てのピン設定をクリア
    for (const auto& pair : pin_configs_) {
        gpio_reset_pin(pair.first);
        pin_map_.erase(pair.first);
    }
    
    pin_configs_.clear();
    callbacks_.clear();
    
    setState(State::INITIALIZED);
    logInfo("GPIO HALリセット完了");
    return ESP_OK;
}

esp_err_t GpioHal::configurePin(const Config& config) {
    if (!isValidPin(config.pin)) {
        logError("無効なピン番号: %d", config.pin);
        return ESP_ERR_INVALID_ARG;
    }
    
    // GPIO設定構造体を作成
    gpio_config_t gpio_conf = {};
    gpio_conf.pin_bit_mask = (1ULL << config.pin);
    gpio_conf.mode = static_cast<gpio_mode_t>(config.direction);
    gpio_conf.pull_up_en = (config.pull == Pull::PULLUP || config.pull == Pull::PULLUP_PULLDOWN) ? GPIO_PULLUP_ENABLE : GPIO_PULLUP_DISABLE;
    gpio_conf.pull_down_en = (config.pull == Pull::PULLDOWN || config.pull == Pull::PULLUP_PULLDOWN) ? GPIO_PULLDOWN_ENABLE : GPIO_PULLDOWN_DISABLE;
    gpio_conf.intr_type = static_cast<gpio_int_type_t>(config.interrupt);
    
    // GPIO設定を適用
    esp_err_t ret = gpio_config(&gpio_conf);
    if (ret != ESP_OK) {
        logError("GPIO設定失敗 ピン%d: %s", config.pin, esp_err_to_name(ret));
        return ret;
    }
    
    // 論理反転設定
    if (config.invert) {
        ret = gpio_set_intr_type(config.pin, GPIO_INTR_ANYEDGE);
        if (ret != ESP_OK) {
            logWarning("論理反転設定警告 ピン%d: %s", config.pin, esp_err_to_name(ret));
        }
    }
    
    // 設定を保存
    pin_configs_[config.pin] = config;
    pin_map_[config.pin] = this;
    
    logDebug("GPIO設定完了 ピン%d 方向:%d プル:%d 割り込み:%d", 
             config.pin, static_cast<int>(config.direction), 
             static_cast<int>(config.pull), static_cast<int>(config.interrupt));
    
    return ESP_OK;
}

esp_err_t GpioHal::digitalWrite(gpio_num_t pin, bool level) {
    if (pin_configs_.find(pin) == pin_configs_.end()) {
        logError("未設定のピン: %d", pin);
        return ESP_ERR_INVALID_ARG;
    }
    
    const Config& config = pin_configs_[pin];
    if (config.direction == Direction::INPUT) {
        logError("入力ピンに出力しようとしました: %d", pin);
        return ESP_ERR_INVALID_STATE;
    }
    
    // 論理反転を考慮
    bool actual_level = config.invert ? !level : level;
    
    esp_err_t ret = gpio_set_level(pin, actual_level ? 1 : 0);
    if (ret != ESP_OK) {
        logError("GPIO出力失敗 ピン%d: %s", pin, esp_err_to_name(ret));
        return ret;
    }
    
    logDebug("GPIO出力 ピン%d レベル:%s", pin, level ? "HIGH" : "LOW");
    return ESP_OK;
}

esp_err_t GpioHal::digitalRead(gpio_num_t pin, bool& level) {
    if (pin_configs_.find(pin) == pin_configs_.end()) {
        logError("未設定のピン: %d", pin);
        return ESP_ERR_INVALID_ARG;
    }
    
    int gpio_level = gpio_get_level(pin);
    
    // 論理反転を考慮
    const Config& config = pin_configs_[pin];
    level = config.invert ? (gpio_level == 0) : (gpio_level != 0);
    
    logDebug("GPIO入力 ピン%d レベル:%s", pin, level ? "HIGH" : "LOW");
    return ESP_OK;
}

esp_err_t GpioHal::setDirection(gpio_num_t pin, Direction direction) {
    auto it = pin_configs_.find(pin);
    if (it == pin_configs_.end()) {
        logError("未設定のピン: %d", pin);
        return ESP_ERR_INVALID_ARG;
    }
    
    // 設定を更新
    it->second.direction = direction;
    
    // GPIOモードを設定
    esp_err_t ret = gpio_set_direction(pin, static_cast<gpio_mode_t>(direction));
    if (ret != ESP_OK) {
        logError("GPIO方向設定失敗 ピン%d: %s", pin, esp_err_to_name(ret));
        return ret;
    }
    
    logDebug("GPIO方向設定 ピン%d 方向:%d", pin, static_cast<int>(direction));
    return ESP_OK;
}

esp_err_t GpioHal::setPull(gpio_num_t pin, Pull pull) {
    auto it = pin_configs_.find(pin);
    if (it == pin_configs_.end()) {
        logError("未設定のピン: %d", pin);
        return ESP_ERR_INVALID_ARG;
    }
    
    // 設定を更新
    it->second.pull = pull;
    
    // プルアップ/プルダウンを設定
    esp_err_t ret = ESP_OK;
    
    if (pull == Pull::PULLUP || pull == Pull::PULLUP_PULLDOWN) {
        ret = gpio_pullup_en(pin);
        if (ret != ESP_OK) {
            logError("プルアップ設定失敗 ピン%d: %s", pin, esp_err_to_name(ret));
            return ret;
        }
    } else {
        ret = gpio_pullup_dis(pin);
    }
    
    if (pull == Pull::PULLDOWN || pull == Pull::PULLUP_PULLDOWN) {
        ret = gpio_pulldown_en(pin);
        if (ret != ESP_OK) {
            logError("プルダウン設定失敗 ピン%d: %s", pin, esp_err_to_name(ret));
            return ret;
        }
    } else {
        ret = gpio_pulldown_dis(pin);
    }
    
    logDebug("GPIOプル設定 ピン%d プル:%d", pin, static_cast<int>(pull));
    return ESP_OK;
}

esp_err_t GpioHal::setInterrupt(gpio_num_t pin, InterruptType type, InterruptCallback callback) {
    if (!isValidPin(pin)) {
        logError("無効なピン番号: %d", pin);
        return ESP_ERR_INVALID_ARG;
    }
    
    // ISRサービスが初期化されているか確認
    if (!isr_service_installed_) {
        esp_err_t ret = installIsrService();
        if (ret != ESP_OK) {
            return ret;
        }
    }
    
    // 割り込みタイプを設定
    esp_err_t ret = gpio_set_intr_type(pin, static_cast<gpio_int_type_t>(type));
    if (ret != ESP_OK) {
        logError("割り込みタイプ設定失敗 ピン%d: %s", pin, esp_err_to_name(ret));
        return ret;
    }
    
    // コールバックを登録
    callbacks_[pin] = callback;
    pin_map_[pin] = this;
    
    // 割り込みハンドラを追加
    ret = gpio_isr_handler_add(pin, gpioIsrHandler, reinterpret_cast<void*>(pin));
    if (ret != ESP_OK) {
        logError("割り込みハンドラ追加失敗 ピン%d: %s", pin, esp_err_to_name(ret));
        callbacks_.erase(pin);
        return ret;
    }
    
    // ピン設定を更新
    if (pin_configs_.find(pin) != pin_configs_.end()) {
        pin_configs_[pin].interrupt = type;
    }
    
    logDebug("GPIO割り込み設定 ピン%d タイプ:%d", pin, static_cast<int>(type));
    return ESP_OK;
}

esp_err_t GpioHal::disableInterrupt(gpio_num_t pin) {
    // 割り込みハンドラを削除
    gpio_isr_handler_remove(pin);
    
    // 割り込みを無効化
    esp_err_t ret = gpio_set_intr_type(pin, GPIO_INTR_DISABLE);
    if (ret != ESP_OK) {
        logWarning("割り込み無効化警告 ピン%d: %s", pin, esp_err_to_name(ret));
    }
    
    // コールバックを削除
    callbacks_.erase(pin);
    
    // ピン設定を更新
    if (pin_configs_.find(pin) != pin_configs_.end()) {
        pin_configs_[pin].interrupt = InterruptType::DISABLE;
    }
    
    logDebug("GPIO割り込み無効化 ピン%d", pin);
    return ESP_OK;
}

bool GpioHal::isValidPin(gpio_num_t pin) {
    return gpio_is_valid_gpio(pin);
}

esp_err_t GpioHal::installIsrService() {
    if (isr_service_installed_) {
        return ESP_OK;
    }
    
    esp_err_t ret = gpio_install_isr_service(ESP_INTR_FLAG_EDGE | ESP_INTR_FLAG_IRAM);
    if (ret != ESP_OK) {
        return ret;
    }
    
    isr_service_installed_ = true;
    return ESP_OK;
}

void IRAM_ATTR GpioHal::gpioIsrHandler(void* arg) {
    gpio_num_t pin = static_cast<gpio_num_t>(reinterpret_cast<intptr_t>(arg));
    
    // ピンに対応するインスタンスを取得
    auto it = pin_map_.find(pin);
    if (it == pin_map_.end()) {
        return;
    }
    
    GpioHal* instance = it->second;
    auto callback_it = instance->callbacks_.find(pin);
    if (callback_it == instance->callbacks_.end()) {
        return;
    }
    
    // 現在のピンレベルを取得
    bool level = gpio_get_level(pin) != 0;
    
    // 論理反転を考慮
    auto config_it = instance->pin_configs_.find(pin);
    if (config_it != instance->pin_configs_.end() && config_it->second.invert) {
        level = !level;
    }
    
    // コールバック実行
    callback_it->second(pin, level);
}

} // namespace hal