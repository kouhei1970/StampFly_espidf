/*
 * HAL Base Class Implementation
 * 
 * HAL基底クラスの実装
 * 
 * 作成者: Kouhei Ito
 * ライセンス: MIT License
 * 
 * Copyright (c) 2025 Kouhei Ito
 */

#include "hal_base.hpp"
#include <cstdarg>
#include <cstdio>

namespace hal {

HalBase::HalBase(const char* component_name)
    : component_name_(component_name)
    , state_(State::UNINITIALIZED)
    , priority_(Priority::NORMAL)
{
    logDebug("HAL基底クラス作成: %s", component_name_);
}

HalBase::~HalBase() {
    logDebug("HAL基底クラス破棄: %s", component_name_);
}

void HalBase::setState(State new_state) {
    if (state_ != new_state) {
        State old_state = state_;
        state_ = new_state;
        
        // 状態変更をログ出力
        const char* state_names[] = {
            "未初期化", "初期化中", "初期化完了", "動作中", "エラー", "中断"
        };
        
        logInfo("状態変更: %s -> %s", 
            state_names[static_cast<int>(old_state)],
            state_names[static_cast<int>(new_state)]);
    }
}

void HalBase::logError(const char* format, ...) {
    char buffer[256];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    ESP_LOGE(component_name_, "%s", buffer);
}

void HalBase::logWarning(const char* format, ...) {
    char buffer[256];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    ESP_LOGW(component_name_, "%s", buffer);
}

void HalBase::logInfo(const char* format, ...) {
    char buffer[256];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    ESP_LOGI(component_name_, "%s", buffer);
}

void HalBase::logDebug(const char* format, ...) {
    char buffer[256];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    ESP_LOGD(component_name_, "%s", buffer);
}

} // namespace hal