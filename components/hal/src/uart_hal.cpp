/*
 * UART HAL Class Implementation
 * 
 * UART通信用のハードウェア抽象化レイヤー実装
 * 
 * 作成者: Kouhei Ito
 * ライセンス: MIT License
 * 
 * Copyright (c) 2025 Kouhei Ito
 */

#include "uart_hal.hpp"
#include "esp_log.h"
#include <cstring>

namespace hal {

UartHal::UartHal(uart_port_t port) 
    : HalBase("UART_HAL")
    , driver_installed_(false)
    , event_queue_(nullptr)
    , event_task_(nullptr) {
    config_.port = port;
    config_.baudrate = 115200;
    config_.data_bits = UART_DATA_8_BITS;
    config_.parity = Parity::NONE;
    config_.stop_bits = StopBits::BITS_1;
    config_.flow_control = FlowControl::NONE;
    config_.tx_pin = UART_PIN_NO_CHANGE;
    config_.rx_pin = UART_PIN_NO_CHANGE;
    config_.rts_pin = UART_PIN_NO_CHANGE;
    config_.cts_pin = UART_PIN_NO_CHANGE;
    config_.rx_buffer_size = 2048;
    config_.tx_buffer_size = 0;
    config_.queue_size = 20;
    
    logDebug("UART HALクラス作成 ポート:%d", static_cast<int>(port));
}

UartHal::~UartHal() {
    if (event_task_) {
        vTaskDelete(event_task_);
        event_task_ = nullptr;
    }
    
    if (driver_installed_) {
        uart_driver_delete(config_.port);
        logDebug("UARTドライバ削除 ポート:%d", static_cast<int>(config_.port));
    }
    
    logDebug("UART HALクラス破棄");
}

esp_err_t UartHal::initialize() {
    setState(State::INITIALIZING);
    setState(State::INITIALIZED);
    logInfo("UART HAL初期化完了 ポート:%d", static_cast<int>(config_.port));
    return ESP_OK;
}

esp_err_t UartHal::configure() {
    if (!isInitialized()) {
        logError("UART HALが初期化されていません");
        return ESP_ERR_INVALID_STATE;
    }
    
    // 既存のドライバを削除
    if (driver_installed_) {
        if (event_task_) {
            vTaskDelete(event_task_);
            event_task_ = nullptr;
        }
        
        esp_err_t ret = uart_driver_delete(config_.port);
        if (ret != ESP_OK) {
            logWarning("UARTドライバ削除警告: %s", esp_err_to_name(ret));
        }
        driver_installed_ = false;
    }
    
    // UART設定構造体を作成
    uart_config_t uart_config = {};
    uart_config.baud_rate = config_.baudrate;
    uart_config.data_bits = config_.data_bits;
    uart_config.parity = static_cast<uart_parity_t>(config_.parity);
    uart_config.stop_bits = static_cast<uart_stop_bits_t>(config_.stop_bits);
    uart_config.flow_ctrl = static_cast<uart_hw_flowcontrol_t>(config_.flow_control);
    uart_config.rx_flow_ctrl_thresh = 122;
    uart_config.source_clk = UART_SCLK_DEFAULT;
    
    // UART設定を適用
    esp_err_t ret = uart_param_config(config_.port, &uart_config);
    if (ret != ESP_OK) {
        logError("UART設定失敗: %s", esp_err_to_name(ret));
        setState(State::ERROR);
        return ret;
    }
    
    // ピン設定
    ret = uart_set_pin(config_.port, config_.tx_pin, config_.rx_pin, 
                       config_.rts_pin, config_.cts_pin);
    if (ret != ESP_OK) {
        logError("UARTピン設定失敗: %s", esp_err_to_name(ret));
        setState(State::ERROR);
        return ret;
    }
    
    // UARTドライバをインストール
    ret = uart_driver_install(config_.port, config_.rx_buffer_size, 
                             config_.tx_buffer_size, config_.queue_size, 
                             &event_queue_, 0);
    if (ret != ESP_OK) {
        logError("UARTドライバインストール失敗: %s", esp_err_to_name(ret));
        setState(State::ERROR);
        return ret;
    }
    
    driver_installed_ = true;
    
    // イベントタスクを作成
    if (event_queue_ && event_callback_) {
        xTaskCreate(eventTask, "uart_event_task", 2048, this, 10, &event_task_);
    }
    
    logInfo("UART設定完了 ポート:%d ボーレート:%d TX:%d RX:%d", 
            static_cast<int>(config_.port), config_.baudrate, 
            config_.tx_pin, config_.rx_pin);
    
    return ESP_OK;
}

esp_err_t UartHal::start() {
    if (!isInitialized()) {
        return ESP_ERR_INVALID_STATE;
    }
    
    if (!driver_installed_) {
        esp_err_t ret = configure();
        if (ret != ESP_OK) {
            return ret;
        }
    }
    
    setState(State::RUNNING);
    logInfo("UART HAL開始");
    return ESP_OK;
}

esp_err_t UartHal::stop() {
    setState(State::SUSPENDED);
    logInfo("UART HAL停止");
    return ESP_OK;
}

esp_err_t UartHal::reset() {
    if (driver_installed_) {
        uart_flush(config_.port);
    }
    
    setState(State::INITIALIZED);
    logInfo("UART HALリセット完了");
    return ESP_OK;
}

esp_err_t UartHal::setConfig(const Config& config) {
    std::lock_guard<std::mutex> lock(mutex_);
    config_ = config;
    
    logDebug("UART設定更新 ポート:%d ボーレート:%d", 
             static_cast<int>(config_.port), config_.baudrate);
    
    return ESP_OK;
}

esp_err_t UartHal::write(const std::vector<uint8_t>& data, TickType_t timeout) {
    if (!isRunning()) {
        logError("UART HALが動作していません");
        return ESP_ERR_INVALID_STATE;
    }
    
    if (data.empty()) {
        return ESP_OK;
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    int written = uart_write_bytes(config_.port, data.data(), data.size());
    if (written < 0) {
        logError("UART書き込み失敗");
        return ESP_FAIL;
    }
    
    if (written != static_cast<int>(data.size())) {
        logWarning("UART部分書き込み: %d/%zu バイト", written, data.size());
    }
    
    logDebug("UART書き込み成功: %d バイト", written);
    return ESP_OK;
}

esp_err_t UartHal::writeString(const std::string& str, TickType_t timeout) {
    std::vector<uint8_t> data(str.begin(), str.end());
    return write(data, timeout);
}

size_t UartHal::read(std::vector<uint8_t>& data, size_t max_length, TickType_t timeout) {
    if (!isRunning()) {
        logError("UART HALが動作していません");
        return 0;
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    data.resize(max_length);
    int length = uart_read_bytes(config_.port, data.data(), max_length, timeout);
    
    if (length < 0) {
        logError("UART読み取り失敗");
        data.clear();
        return 0;
    }
    
    data.resize(length);
    logDebug("UART読み取り成功: %d バイト", length);
    return length;
}

bool UartHal::readLine(std::string& line, TickType_t timeout) {
    if (!isRunning()) {
        return false;
    }
    
    line.clear();
    uint8_t byte;
    TickType_t start_time = xTaskGetTickCount();
    
    while (true) {
        int length = uart_read_bytes(config_.port, &byte, 1, 10 / portTICK_PERIOD_MS);
        
        if (length == 1) {
            if (byte == '\n') {
                // 改行文字で終了
                if (!line.empty() && line.back() == '\r') {
                    line.pop_back(); // CRを削除
                }
                return true;
            }
            line.push_back(byte);
        }
        
        // タイムアウトチェック
        if (timeout != portMAX_DELAY && 
            (xTaskGetTickCount() - start_time) >= timeout) {
            return false;
        }
    }
}

size_t UartHal::available() {
    if (!isRunning()) {
        return 0;
    }
    
    size_t length = 0;
    uart_get_buffered_data_len(config_.port, &length);
    return length;
}

esp_err_t UartHal::flush() {
    if (!isRunning()) {
        return ESP_ERR_INVALID_STATE;
    }
    
    return uart_flush(config_.port);
}

esp_err_t UartHal::waitTxDone(TickType_t timeout) {
    if (!isRunning()) {
        return ESP_ERR_INVALID_STATE;
    }
    
    return uart_wait_tx_done(config_.port, timeout);
}

esp_err_t UartHal::setBaudrate(uint32_t baudrate) {
    if (!driver_installed_) {
        config_.baudrate = baudrate;
        return ESP_OK;
    }
    
    esp_err_t ret = uart_set_baudrate(config_.port, baudrate);
    if (ret == ESP_OK) {
        config_.baudrate = baudrate;
        logInfo("ボーレート変更: %d", baudrate);
    }
    
    return ret;
}

esp_err_t UartHal::getBaudrate(uint32_t& baudrate) {
    if (!driver_installed_) {
        baudrate = config_.baudrate;
        return ESP_OK;
    }
    
    return uart_get_baudrate(config_.port, &baudrate);
}

esp_err_t UartHal::sendBreak(int duration) {
    if (!isRunning()) {
        return ESP_ERR_INVALID_STATE;
    }
    
    return uart_set_break(config_.port, duration);
}

esp_err_t UartHal::setPatternDetect(const char* pattern, size_t pattern_length, 
                                   int gap_timeout, int pre_idle, int post_idle) {
    if (!driver_installed_) {
        return ESP_ERR_INVALID_STATE;
    }
    
    return uart_enable_pattern_det_baud_intr(config_.port, pattern[0], 
                                            pattern_length, gap_timeout, 
                                            pre_idle, post_idle);
}

esp_err_t UartHal::disablePatternDetect() {
    if (!driver_installed_) {
        return ESP_ERR_INVALID_STATE;
    }
    
    return uart_disable_pattern_det_intr(config_.port);
}

esp_err_t UartHal::setEventCallback(EventCallback callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    event_callback_ = callback;
    
    // イベントタスクを再作成
    if (driver_installed_ && event_queue_ && callback) {
        if (event_task_) {
            vTaskDelete(event_task_);
        }
        xTaskCreate(eventTask, "uart_event_task", 2048, this, 10, &event_task_);
    }
    
    return ESP_OK;
}

esp_err_t UartHal::setRS485Mode(bool enable, int tx_time, int rx_time) {
    if (!driver_installed_) {
        return ESP_ERR_INVALID_STATE;
    }
    
    if (enable) {
        return uart_set_mode(config_.port, UART_MODE_RS485_HALF_DUPLEX);
    } else {
        return uart_set_mode(config_.port, UART_MODE_UART);
    }
}

void UartHal::eventTask(void* arg) {
    UartHal* instance = static_cast<UartHal*>(arg);
    uart_event_t event;
    
    while (true) {
        if (xQueueReceive(instance->event_queue_, &event, portMAX_DELAY)) {
            if (instance->event_callback_) {
                instance->event_callback_(
                    static_cast<EventType>(event.type), 
                    event.size
                );
            }
        }
    }
}

} // namespace hal