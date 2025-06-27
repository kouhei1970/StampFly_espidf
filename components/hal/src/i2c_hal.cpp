/*
 * I2C HAL Class Implementation
 * 
 * I2C通信用のハードウェア抽象化レイヤー実装
 * ESP-IDF I2C APIをC++クラスでラップ
 * 
 * 作成者: Kouhei Ito
 * ライセンス: MIT License
 * 
 * Copyright (c) 2025 Kouhei Ito
 */

#include "i2c_hal.hpp"
#include "esp_log.h"
#include <cstring>

namespace hal {

I2cHal::I2cHal(i2c_port_t port) 
    : HalBase("I2C_HAL")
    , driver_installed_(false) {
    config_.port = port;
    config_.mode = Mode::MASTER;
    config_.sda_pin = GPIO_NUM_NC;
    config_.scl_pin = GPIO_NUM_NC;
    config_.frequency = 100000; // 100kHz デフォルト
    config_.sda_pullup_enable = true;
    config_.scl_pullup_enable = true;
    config_.slave_address = 0;
    
    logDebug("I2C HALクラス作成 ポート:%d", static_cast<int>(port));
}

I2cHal::~I2cHal() {
    if (driver_installed_) {
        i2c_driver_delete(config_.port);
        logDebug("I2Cドライバ削除 ポート:%d", static_cast<int>(config_.port));
    }
    logDebug("I2C HALクラス破棄");
}

esp_err_t I2cHal::initialize() {
    setState(State::INITIALIZING);
    
    if (config_.sda_pin == GPIO_NUM_NC || config_.scl_pin == GPIO_NUM_NC) {
        logError("I2Cピンが設定されていません SDA:%d SCL:%d", config_.sda_pin, config_.scl_pin);
        setState(State::ERROR);
        return ESP_ERR_INVALID_ARG;
    }
    
    setState(State::INITIALIZED);
    logInfo("I2C HAL初期化完了 ポート:%d", static_cast<int>(config_.port));
    return ESP_OK;
}

esp_err_t I2cHal::configure() {
    if (!isInitialized()) {
        logError("I2C HALが初期化されていません");
        return ESP_ERR_INVALID_STATE;
    }
    
    // 既存のドライバを削除
    if (driver_installed_) {
        esp_err_t ret = i2c_driver_delete(config_.port);
        if (ret != ESP_OK) {
            logWarning("I2Cドライバ削除警告: %s", esp_err_to_name(ret));
        }
        driver_installed_ = false;
    }
    
    // I2C設定構造体を作成
    i2c_config_t i2c_conf = {};
    i2c_conf.mode = static_cast<i2c_mode_t>(config_.mode);
    i2c_conf.sda_io_num = config_.sda_pin;
    i2c_conf.scl_io_num = config_.scl_pin;
    i2c_conf.sda_pullup_en = config_.sda_pullup_enable;
    i2c_conf.scl_pullup_en = config_.scl_pullup_enable;
    
    if (config_.mode == Mode::MASTER) {
        i2c_conf.master.clk_speed = config_.frequency;
    } else {
        i2c_conf.slave.addr_10bit_en = 0;
        i2c_conf.slave.slave_addr = config_.slave_address;
        i2c_conf.slave.maximum_speed = config_.frequency;
    }
    
    // I2C設定を適用
    esp_err_t ret = i2c_param_config(config_.port, &i2c_conf);
    if (ret != ESP_OK) {
        logError("I2C設定失敗: %s", esp_err_to_name(ret));
        setState(State::ERROR);
        return ret;
    }
    
    // I2Cドライバをインストール
    ret = i2c_driver_install(config_.port, static_cast<i2c_mode_t>(config_.mode), 0, 0, 0);
    if (ret != ESP_OK) {
        logError("I2Cドライバインストール失敗: %s", esp_err_to_name(ret));
        setState(State::ERROR);
        return ret;
    }
    
    driver_installed_ = true;
    
    logInfo("I2C設定完了 ポート:%d 周波数:%dHz SDA:%d SCL:%d", 
            static_cast<int>(config_.port), config_.frequency, config_.sda_pin, config_.scl_pin);
    
    return ESP_OK;
}

esp_err_t I2cHal::start() {
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
    logInfo("I2C HAL開始");
    return ESP_OK;
}

esp_err_t I2cHal::stop() {
    setState(State::SUSPENDED);
    logInfo("I2C HAL停止");
    return ESP_OK;
}

esp_err_t I2cHal::reset() {
    if (driver_installed_) {
        i2c_driver_delete(config_.port);
        driver_installed_ = false;
    }
    
    setState(State::INITIALIZED);
    logInfo("I2C HALリセット完了");
    return ESP_OK;
}

esp_err_t I2cHal::setConfig(const Config& config) {
    std::lock_guard<std::mutex> lock(mutex_);
    config_ = config;
    
    logDebug("I2C設定更新 ポート:%d 周波数:%dHz", 
             static_cast<int>(config_.port), config_.frequency);
    
    return ESP_OK;
}

esp_err_t I2cHal::write(uint8_t device_address, const std::vector<uint8_t>& data, TickType_t timeout) {
    if (!isRunning()) {
        logError("I2C HALが動作していません");
        return ESP_ERR_INVALID_STATE;
    }
    
    if (data.empty()) {
        logError("書き込みデータが空です");
        return ESP_ERR_INVALID_ARG;
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    // I2Cコマンドリンクを作成
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    if (cmd == nullptr) {
        logError("I2Cコマンドリンク作成失敗");
        return ESP_ERR_NO_MEM;
    }
    
    // I2C書き込みシーケンス
    esp_err_t ret = i2c_master_start(cmd);
    ret |= i2c_master_write_byte(cmd, (device_address << 1) | I2C_MASTER_WRITE, true);
    ret |= i2c_master_write(cmd, data.data(), data.size(), true);
    ret |= i2c_master_stop(cmd);
    
    if (ret != ESP_OK) {
        i2c_cmd_link_delete(cmd);
        logError("I2Cコマンド構築失敗: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // コマンド実行
    ret = i2c_master_cmd_begin(config_.port, cmd, timeout);
    i2c_cmd_link_delete(cmd);
    
    if (ret != ESP_OK) {
        logError("I2C書き込み失敗 アドレス:0x%02X サイズ:%zu エラー:%s", 
                 device_address, data.size(), esp_err_to_name(ret));
        return ret;
    }
    
    logDebug("I2C書き込み成功 アドレス:0x%02X サイズ:%zu", device_address, data.size());
    return ESP_OK;
}

esp_err_t I2cHal::read(uint8_t device_address, std::vector<uint8_t>& data, size_t length, TickType_t timeout) {
    if (!isRunning()) {
        logError("I2C HALが動作していません");
        return ESP_ERR_INVALID_STATE;
    }
    
    if (length == 0) {
        logError("読み取りサイズが0です");
        return ESP_ERR_INVALID_ARG;
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    // データバッファを準備
    data.resize(length);
    
    // I2Cコマンドリンクを作成
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    if (cmd == nullptr) {
        logError("I2Cコマンドリンク作成失敗");
        return ESP_ERR_NO_MEM;
    }
    
    // I2C読み取りシーケンス
    esp_err_t ret = i2c_master_start(cmd);
    ret |= i2c_master_write_byte(cmd, (device_address << 1) | I2C_MASTER_READ, true);
    
    if (length > 1) {
        ret |= i2c_master_read(cmd, data.data(), length - 1, I2C_MASTER_ACK);
    }
    ret |= i2c_master_read_byte(cmd, &data[length - 1], I2C_MASTER_NACK);
    ret |= i2c_master_stop(cmd);
    
    if (ret != ESP_OK) {
        i2c_cmd_link_delete(cmd);
        logError("I2Cコマンド構築失敗: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // コマンド実行
    ret = i2c_master_cmd_begin(config_.port, cmd, timeout);
    i2c_cmd_link_delete(cmd);
    
    if (ret != ESP_OK) {
        logError("I2C読み取り失敗 アドレス:0x%02X サイズ:%zu エラー:%s", 
                 device_address, length, esp_err_to_name(ret));
        data.clear();
        return ret;
    }
    
    logDebug("I2C読み取り成功 アドレス:0x%02X サイズ:%zu", device_address, length);
    return ESP_OK;
}

esp_err_t I2cHal::writeRegister(uint8_t device_address, uint8_t register_address, 
                               const std::vector<uint8_t>& data, TickType_t timeout) {
    if (!isRunning()) {
        logError("I2C HALが動作していません");
        return ESP_ERR_INVALID_STATE;
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    // I2Cコマンドリンクを作成
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    if (cmd == nullptr) {
        logError("I2Cコマンドリンク作成失敗");
        return ESP_ERR_NO_MEM;
    }
    
    // I2Cレジスタ書き込みシーケンス
    esp_err_t ret = i2c_master_start(cmd);
    ret |= i2c_master_write_byte(cmd, (device_address << 1) | I2C_MASTER_WRITE, true);
    ret |= i2c_master_write_byte(cmd, register_address, true);
    
    if (!data.empty()) {
        ret |= i2c_master_write(cmd, data.data(), data.size(), true);
    }
    
    ret |= i2c_master_stop(cmd);
    
    if (ret != ESP_OK) {
        i2c_cmd_link_delete(cmd);
        logError("I2Cコマンド構築失敗: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // コマンド実行
    ret = i2c_master_cmd_begin(config_.port, cmd, timeout);
    i2c_cmd_link_delete(cmd);
    
    if (ret != ESP_OK) {
        logError("I2Cレジスタ書き込み失敗 アドレス:0x%02X レジスタ:0x%02X エラー:%s", 
                 device_address, register_address, esp_err_to_name(ret));
        return ret;
    }
    
    logDebug("I2Cレジスタ書き込み成功 アドレス:0x%02X レジスタ:0x%02X サイズ:%zu", 
             device_address, register_address, data.size());
    return ESP_OK;
}

esp_err_t I2cHal::readRegister(uint8_t device_address, uint8_t register_address, 
                              std::vector<uint8_t>& data, size_t length, TickType_t timeout) {
    if (!isRunning()) {
        logError("I2C HALが動作していません");
        return ESP_ERR_INVALID_STATE;
    }
    
    if (length == 0) {
        logError("読み取りサイズが0です");
        return ESP_ERR_INVALID_ARG;
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    // データバッファを準備
    data.resize(length);
    
    // I2Cコマンドリンクを作成
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    if (cmd == nullptr) {
        logError("I2Cコマンドリンク作成失敗");
        return ESP_ERR_NO_MEM;
    }
    
    // I2Cレジスタ読み取りシーケンス（書き込み + 読み取り）
    esp_err_t ret = i2c_master_start(cmd);
    ret |= i2c_master_write_byte(cmd, (device_address << 1) | I2C_MASTER_WRITE, true);
    ret |= i2c_master_write_byte(cmd, register_address, true);
    
    ret |= i2c_master_start(cmd); // リピートスタート
    ret |= i2c_master_write_byte(cmd, (device_address << 1) | I2C_MASTER_READ, true);
    
    if (length > 1) {
        ret |= i2c_master_read(cmd, data.data(), length - 1, I2C_MASTER_ACK);
    }
    ret |= i2c_master_read_byte(cmd, &data[length - 1], I2C_MASTER_NACK);
    ret |= i2c_master_stop(cmd);
    
    if (ret != ESP_OK) {
        i2c_cmd_link_delete(cmd);
        logError("I2Cコマンド構築失敗: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // コマンド実行
    ret = i2c_master_cmd_begin(config_.port, cmd, timeout);
    i2c_cmd_link_delete(cmd);
    
    if (ret != ESP_OK) {
        logError("I2Cレジスタ読み取り失敗 アドレス:0x%02X レジスタ:0x%02X エラー:%s", 
                 device_address, register_address, esp_err_to_name(ret));
        data.clear();
        return ret;
    }
    
    logDebug("I2Cレジスタ読み取り成功 アドレス:0x%02X レジスタ:0x%02X サイズ:%zu", 
             device_address, register_address, length);
    return ESP_OK;
}

esp_err_t I2cHal::writeRegister8(uint8_t device_address, uint8_t register_address, 
                                uint8_t value, TickType_t timeout) {
    std::vector<uint8_t> data = {value};
    return writeRegister(device_address, register_address, data, timeout);
}

esp_err_t I2cHal::readRegister8(uint8_t device_address, uint8_t register_address, 
                               uint8_t& value, TickType_t timeout) {
    std::vector<uint8_t> data;
    esp_err_t ret = readRegister(device_address, register_address, data, 1, timeout);
    if (ret == ESP_OK && !data.empty()) {
        value = data[0];
    }
    return ret;
}

esp_err_t I2cHal::writeRegister16(uint8_t device_address, uint8_t register_address, 
                                 uint16_t value, bool big_endian, TickType_t timeout) {
    std::vector<uint8_t> data(2);
    if (big_endian) {
        data[0] = (value >> 8) & 0xFF;  // 上位バイト
        data[1] = value & 0xFF;         // 下位バイト
    } else {
        data[0] = value & 0xFF;         // 下位バイト
        data[1] = (value >> 8) & 0xFF;  // 上位バイト
    }
    return writeRegister(device_address, register_address, data, timeout);
}

esp_err_t I2cHal::readRegister16(uint8_t device_address, uint8_t register_address, 
                                uint16_t& value, bool big_endian, TickType_t timeout) {
    std::vector<uint8_t> data;
    esp_err_t ret = readRegister(device_address, register_address, data, 2, timeout);
    if (ret == ESP_OK && data.size() >= 2) {
        if (big_endian) {
            value = (static_cast<uint16_t>(data[0]) << 8) | data[1];
        } else {
            value = (static_cast<uint16_t>(data[1]) << 8) | data[0];
        }
    }
    return ret;
}

bool I2cHal::deviceExists(uint8_t device_address, TickType_t timeout) {
    if (!isRunning()) {
        return false;
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    // I2Cコマンドリンクを作成
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    if (cmd == nullptr) {
        return false;
    }
    
    // デバイス存在確認シーケンス
    esp_err_t ret = i2c_master_start(cmd);
    ret |= i2c_master_write_byte(cmd, (device_address << 1) | I2C_MASTER_WRITE, true);
    ret |= i2c_master_stop(cmd);
    
    if (ret != ESP_OK) {
        i2c_cmd_link_delete(cmd);
        return false;
    }
    
    // コマンド実行
    ret = i2c_master_cmd_begin(config_.port, cmd, timeout);
    i2c_cmd_link_delete(cmd);
    
    bool exists = (ret == ESP_OK);
    logDebug("I2Cデバイス存在確認 アドレス:0x%02X 結果:%s", 
             device_address, exists ? "存在" : "不在");
    
    return exists;
}

esp_err_t I2cHal::scanBus(std::vector<uint8_t>& found_devices) {
    if (!isRunning()) {
        logError("I2C HALが動作していません");
        return ESP_ERR_INVALID_STATE;
    }
    
    found_devices.clear();
    
    logInfo("I2Cバススキャン開始");
    
    // 標準的なI2Cアドレス範囲をスキャン (0x08 - 0x77)
    for (uint8_t addr = 0x08; addr <= 0x77; addr++) {
        if (deviceExists(addr, pdMS_TO_TICKS(100))) {
            found_devices.push_back(addr);
            logInfo("I2Cデバイス発見 アドレス:0x%02X", addr);
        }
    }
    
    logInfo("I2Cバススキャン完了 発見デバイス数:%zu", found_devices.size());
    
    return ESP_OK;
}

} // namespace hal