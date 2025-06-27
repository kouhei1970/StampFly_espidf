/*
 * Interrupt HAL Class Implementation
 * 
 * 割り込み管理用のハードウェア抽象化レイヤー実装
 * 
 * 作成者: Kouhei Ito
 * ライセンス: MIT License
 * 
 * Copyright (c) 2025 Kouhei Ito
 */

#include "interrupt_hal.hpp"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace hal {

InterruptHal::InterruptHal() 
    : HalBase("INTERRUPT_HAL") {
    portMUX_INITIALIZE(&critical_mux_);
    logDebug("Interrupt HALクラス作成");
}

InterruptHal::~InterruptHal() {
    // 全タイマーを削除
    for (auto& pair : timers_) {
        esp_timer_delete(pair.second.handle);
    }
    timers_.clear();
    
    // 全割り込みを解除
    for (auto& pair : interrupts_) {
        esp_intr_free(pair.second.handle);
    }
    interrupts_.clear();
    
    logDebug("Interrupt HALクラス破棄");
}

esp_err_t InterruptHal::initialize() {
    setState(State::INITIALIZING);
    setState(State::INITIALIZED);
    logInfo("Interrupt HAL初期化完了");
    return ESP_OK;
}

esp_err_t InterruptHal::configure() {
    if (!isInitialized()) {
        logError("Interrupt HALが初期化されていません");
        return ESP_ERR_INVALID_STATE;
    }
    
    logInfo("Interrupt HAL設定完了");
    return ESP_OK;
}

esp_err_t InterruptHal::start() {
    if (!isInitialized()) {
        return ESP_ERR_INVALID_STATE;
    }
    
    setState(State::RUNNING);
    logInfo("Interrupt HAL開始");
    return ESP_OK;
}

esp_err_t InterruptHal::stop() {
    // 全タイマーを停止
    for (auto& pair : timers_) {
        esp_timer_stop(pair.second.handle);
    }
    
    setState(State::SUSPENDED);
    logInfo("Interrupt HAL停止");
    return ESP_OK;
}

esp_err_t InterruptHal::reset() {
    // 統計情報をリセット
    for (auto& pair : timers_) {
        pair.second.stats = {};
    }
    
    for (auto& pair : interrupts_) {
        pair.second.stats = {};
    }
    
    setState(State::INITIALIZED);
    logInfo("Interrupt HALリセット完了");
    return ESP_OK;
}

esp_err_t InterruptHal::createHighResTimer(uint32_t timer_id, const TimerConfig& config, TimerCallback callback) {
    if (!isRunning()) {
        logError("Interrupt HALが動作していません");
        return ESP_ERR_INVALID_STATE;
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    // 既存のタイマーをチェック
    auto it = timers_.find(timer_id);
    if (it != timers_.end()) {
        logError("タイマーID %d は既に使用されています", timer_id);
        return ESP_ERR_INVALID_ARG;
    }
    
    // タイマー設定を作成
    esp_timer_create_args_t timer_args = {};
    timer_args.callback = espTimerCallback;
    timer_args.arg = reinterpret_cast<void*>(timer_id);
    timer_args.name = "hal_timer";
    timer_args.skip_unhandled_events = true;
    
    TimerInfo timer_info;
    timer_info.config = config;
    timer_info.callback = callback;
    timer_info.stats = {};
    
    // タイマーを作成
    esp_err_t ret = esp_timer_create(&timer_args, &timer_info.handle);
    if (ret != ESP_OK) {
        logError("タイマー作成失敗 ID:%d: %s", timer_id, esp_err_to_name(ret));
        return ret;
    }
    
    timers_[timer_id] = timer_info;
    logInfo("高分解能タイマー作成 ID:%d 周期:%lluus", timer_id, config.period_us);
    
    return ESP_OK;
}

esp_err_t InterruptHal::startTimer(uint32_t timer_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = timers_.find(timer_id);
    if (it == timers_.end()) {
        logError("タイマーID %d が見つかりません", timer_id);
        return ESP_ERR_NOT_FOUND;
    }
    
    esp_err_t ret;
    if (it->second.config.auto_reload) {
        ret = esp_timer_start_periodic(it->second.handle, it->second.config.period_us);
    } else {
        ret = esp_timer_start_once(it->second.handle, it->second.config.period_us);
    }
    
    if (ret != ESP_OK) {
        logError("タイマー開始失敗 ID:%d: %s", timer_id, esp_err_to_name(ret));
        return ret;
    }
    
    logInfo("タイマー開始 ID:%d", timer_id);
    return ESP_OK;
}

esp_err_t InterruptHal::stopTimer(uint32_t timer_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = timers_.find(timer_id);
    if (it == timers_.end()) {
        logError("タイマーID %d が見つかりません", timer_id);
        return ESP_ERR_NOT_FOUND;
    }
    
    esp_err_t ret = esp_timer_stop(it->second.handle);
    if (ret != ESP_OK) {
        logError("タイマー停止失敗 ID:%d: %s", timer_id, esp_err_to_name(ret));
        return ret;
    }
    
    logInfo("タイマー停止 ID:%d", timer_id);
    return ESP_OK;
}

esp_err_t InterruptHal::deleteTimer(uint32_t timer_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = timers_.find(timer_id);
    if (it == timers_.end()) {
        logError("タイマーID %d が見つかりません", timer_id);
        return ESP_ERR_NOT_FOUND;
    }
    
    // タイマーを停止してから削除
    esp_timer_stop(it->second.handle);
    esp_err_t ret = esp_timer_delete(it->second.handle);
    if (ret != ESP_OK) {
        logError("タイマー削除失敗 ID:%d: %s", timer_id, esp_err_to_name(ret));
        return ret;
    }
    
    timers_.erase(it);
    logInfo("タイマー削除 ID:%d", timer_id);
    
    return ESP_OK;
}

esp_err_t InterruptHal::setTimerPeriod(uint32_t timer_id, uint64_t period_us) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = timers_.find(timer_id);
    if (it == timers_.end()) {
        logError("タイマーID %d が見つかりません", timer_id);
        return ESP_ERR_NOT_FOUND;
    }
    
    // タイマーを停止
    esp_timer_stop(it->second.handle);
    
    // 設定を更新
    it->second.config.period_us = period_us;
    
    // タイマーを再開
    esp_err_t ret;
    if (it->second.config.auto_reload) {
        ret = esp_timer_start_periodic(it->second.handle, period_us);
    } else {
        ret = esp_timer_start_once(it->second.handle, period_us);
    }
    
    if (ret != ESP_OK) {
        logError("タイマー周期変更失敗 ID:%d: %s", timer_id, esp_err_to_name(ret));
        return ret;
    }
    
    logInfo("タイマー周期変更 ID:%d 新周期:%lluus", timer_id, period_us);
    return ESP_OK;
}

esp_err_t InterruptHal::registerInterrupt(uint32_t interrupt_id, const SourceConfig& config, InterruptHandler handler) {
    if (!isRunning()) {
        logError("Interrupt HALが動作していません");
        return ESP_ERR_INVALID_STATE;
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    // 既存の割り込みをチェック
    auto it = interrupts_.find(interrupt_id);
    if (it != interrupts_.end()) {
        logError("割り込みID %d は既に使用されています", interrupt_id);
        return ESP_ERR_INVALID_ARG;
    }
    
    InterruptInfo interrupt_info;
    interrupt_info.config = config;
    interrupt_info.handler = handler;
    interrupt_info.stats = {};
    
    // 割り込みを登録
    esp_err_t ret = esp_intr_alloc(config.source, config.flags, 
                                  interruptHandlerWrapper, 
                                  reinterpret_cast<void*>(interrupt_id), 
                                  &interrupt_info.handle);
    if (ret != ESP_OK) {
        logError("割り込み登録失敗 ID:%d ソース:%d: %s", interrupt_id, config.source, esp_err_to_name(ret));
        return ret;
    }
    
    interrupts_[interrupt_id] = interrupt_info;
    logInfo("割り込み登録 ID:%d ソース:%d 優先度:%d", 
            interrupt_id, config.source, static_cast<int>(config.priority));
    
    return ESP_OK;
}

esp_err_t InterruptHal::unregisterInterrupt(uint32_t interrupt_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = interrupts_.find(interrupt_id);
    if (it == interrupts_.end()) {
        logError("割り込みID %d が見つかりません", interrupt_id);
        return ESP_ERR_NOT_FOUND;
    }
    
    esp_err_t ret = esp_intr_free(it->second.handle);
    if (ret != ESP_OK) {
        logError("割り込み解除失敗 ID:%d: %s", interrupt_id, esp_err_to_name(ret));
        return ret;
    }
    
    interrupts_.erase(it);
    logInfo("割り込み解除 ID:%d", interrupt_id);
    
    return ESP_OK;
}

esp_err_t InterruptHal::enableInterrupt(uint32_t interrupt_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = interrupts_.find(interrupt_id);
    if (it == interrupts_.end()) {
        logError("割り込みID %d が見つかりません", interrupt_id);
        return ESP_ERR_NOT_FOUND;
    }
    
    esp_err_t ret = esp_intr_enable(it->second.handle);
    if (ret != ESP_OK) {
        logError("割り込み有効化失敗 ID:%d: %s", interrupt_id, esp_err_to_name(ret));
        return ret;
    }
    
    logDebug("割り込み有効化 ID:%d", interrupt_id);
    return ESP_OK;
}

esp_err_t InterruptHal::disableInterrupt(uint32_t interrupt_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = interrupts_.find(interrupt_id);
    if (it == interrupts_.end()) {
        logError("割り込みID %d が見つかりません", interrupt_id);
        return ESP_ERR_NOT_FOUND;
    }
    
    esp_err_t ret = esp_intr_disable(it->second.handle);
    if (ret != ESP_OK) {
        logError("割り込み無効化失敗 ID:%d: %s", interrupt_id, esp_err_to_name(ret));
        return ret;
    }
    
    logDebug("割り込み無効化 ID:%d", interrupt_id);
    return ESP_OK;
}

uint32_t InterruptHal::disableAllInterrupts() {
    return portSET_INTERRUPT_MASK_FROM_ISR();
}

void InterruptHal::restoreInterrupts(uint32_t state) {
    portCLEAR_INTERRUPT_MASK_FROM_ISR(state);
}

esp_err_t InterruptHal::setPriority(uint32_t interrupt_id, Priority priority) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = interrupts_.find(interrupt_id);
    if (it == interrupts_.end()) {
        logError("割り込みID %d が見つかりません", interrupt_id);
        return ESP_ERR_NOT_FOUND;
    }
    
    // 新しい優先度で割り込みを再登録
    esp_intr_free(it->second.handle);
    
    it->second.config.priority = priority;
    it->second.config.flags |= static_cast<int>(priority);
    
    esp_err_t ret = esp_intr_alloc(it->second.config.source, it->second.config.flags,
                                  interruptHandlerWrapper, 
                                  reinterpret_cast<void*>(interrupt_id),
                                  &it->second.handle);
    if (ret != ESP_OK) {
        logError("割り込み優先度変更失敗 ID:%d: %s", interrupt_id, esp_err_to_name(ret));
        return ret;
    }
    
    logInfo("割り込み優先度変更 ID:%d 新優先度:%d", interrupt_id, static_cast<int>(priority));
    return ESP_OK;
}

esp_err_t InterruptHal::setCpuAffinity(uint32_t interrupt_id, uint32_t cpu_mask) {
    // ESP32-S3では特定CPUへの割り込み親和性設定は限定的
    logWarning("CPU親和性設定は部分的にサポートされています ID:%d マスク:0x%x", interrupt_id, cpu_mask);
    return ESP_OK;
}

esp_err_t InterruptHal::getStatistics(uint32_t interrupt_id, Statistics& stats) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // タイマー統計を確認
    auto timer_it = timers_.find(interrupt_id);
    if (timer_it != timers_.end()) {
        stats = timer_it->second.stats;
        return ESP_OK;
    }
    
    // 割り込み統計を確認
    auto interrupt_it = interrupts_.find(interrupt_id);
    if (interrupt_it != interrupts_.end()) {
        stats = interrupt_it->second.stats;
        return ESP_OK;
    }
    
    logError("ID %d が見つかりません", interrupt_id);
    return ESP_ERR_NOT_FOUND;
}

esp_err_t InterruptHal::resetStatistics(uint32_t interrupt_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // タイマー統計をリセット
    auto timer_it = timers_.find(interrupt_id);
    if (timer_it != timers_.end()) {
        timer_it->second.stats = {};
        logInfo("タイマー統計リセット ID:%d", interrupt_id);
        return ESP_OK;
    }
    
    // 割り込み統計をリセット
    auto interrupt_it = interrupts_.find(interrupt_id);
    if (interrupt_it != interrupts_.end()) {
        interrupt_it->second.stats = {};
        logInfo("割り込み統計リセット ID:%d", interrupt_id);
        return ESP_OK;
    }
    
    logError("ID %d が見つかりません", interrupt_id);
    return ESP_ERR_NOT_FOUND;
}

int InterruptHal::getCurrentCpu() {
    return xPortGetCoreID();
}

bool InterruptHal::isInIsr() {
    return xPortInIsrContext();
}

portMUX_TYPE* InterruptHal::enterCriticalSection() {
    portENTER_CRITICAL(&critical_mux_);
    return &critical_mux_;
}

void InterruptHal::exitCriticalSection(portMUX_TYPE* mux) {
    portEXIT_CRITICAL(mux);
}

void InterruptHal::espTimerCallback(void* arg) {
    uint32_t timer_id = reinterpret_cast<uintptr_t>(arg);
    
    // インスタンスを静的に取得（簡略化のため）
    // 実際の実装では、より安全な方法でインスタンスを管理する必要があります
    static InterruptHal* instance = nullptr;
    if (!instance) {
        return;
    }
    
    auto it = instance->timers_.find(timer_id);
    if (it != instance->timers_.end()) {
        // 統計更新
        it->second.stats.total_count++;
        
        // コールバック実行
        if (it->second.callback) {
            it->second.callback();
        }
    }
}

void IRAM_ATTR InterruptHal::interruptHandlerWrapper(void* arg) {
    uint32_t interrupt_id = reinterpret_cast<uintptr_t>(arg);
    
    // インスタンスを静的に取得（簡略化のため）
    // 実際の実装では、より安全な方法でインスタンスを管理する必要があります
    static InterruptHal* instance = nullptr;
    if (!instance) {
        return;
    }
    
    auto it = instance->interrupts_.find(interrupt_id);
    if (it != instance->interrupts_.end()) {
        // 統計更新
        it->second.stats.total_count++;
        
        // ハンドラ実行
        if (it->second.handler) {
            it->second.handler();
        }
    }
}

} // namespace hal