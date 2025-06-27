/*
 * SPI HAL Class Implementation
 * 
 * SPI通信用のハードウェア抽象化レイヤー実装
 * ESP-IDF SPI APIをC++クラスでラップ
 * 
 * 作成者: Kouhei Ito
 * ライセンス: MIT License
 * 
 * Copyright (c) 2025 Kouhei Ito
 */

#include "spi_hal.hpp"
#include "esp_log.h"
#include <cstring>
#include <algorithm>

namespace hal {

SpiHal::SpiHal(spi_host_device_t host) 
    : HalBase("SPI_HAL")
    , bus_initialized_(false) {
    config_.host = host;
    config_.mosi_pin = GPIO_NUM_NC;
    config_.miso_pin = GPIO_NUM_NC;
    config_.sclk_pin = GPIO_NUM_NC;
    config_.cs_pin = GPIO_NUM_NC;
    config_.max_transfer_size = 4096;  // デフォルト4KB
    config_.dma_channel = SPI_DMA_CH_AUTO;
    config_.queue_size = 7;
    
    logDebug("SPI HALクラス作成 ホスト:%d", static_cast<int>(host));
}

SpiHal::~SpiHal() {
    // 登録されたデバイスを削除
    for (auto device : devices_) {
        spi_bus_remove_device(device);
        logDebug("SPIデバイス削除 ハンドル:%p", device);
    }
    devices_.clear();
    
    // SPIバスを解放
    if (bus_initialized_) {
        spi_bus_free(config_.host);
        logDebug("SPIバス解放 ホスト:%d", static_cast<int>(config_.host));
    }
    
    logDebug("SPI HALクラス破棄");
}

esp_err_t SpiHal::initialize() {
    setState(State::INITIALIZING);
    
    if (config_.mosi_pin == GPIO_NUM_NC && config_.miso_pin == GPIO_NUM_NC) {
        logError("MOSIまたはMISOピンが設定されていません");
        setState(State::ERROR);
        return ESP_ERR_INVALID_ARG;
    }
    
    if (config_.sclk_pin == GPIO_NUM_NC) {
        logError("SCLKピンが設定されていません");
        setState(State::ERROR);
        return ESP_ERR_INVALID_ARG;
    }
    
    setState(State::INITIALIZED);
    logInfo("SPI HAL初期化完了 ホスト:%d", static_cast<int>(config_.host));
    return ESP_OK;
}

esp_err_t SpiHal::configure() {
    if (!isInitialized()) {
        logError("SPI HALが初期化されていません");
        return ESP_ERR_INVALID_STATE;
    }
    
    // 既存のバスを解放
    if (bus_initialized_) {
        // デバイスを先に削除
        for (auto device : devices_) {
            spi_bus_remove_device(device);
        }
        devices_.clear();
        
        esp_err_t ret = spi_bus_free(config_.host);
        if (ret != ESP_OK) {
            logWarning("SPIバス解放警告: %s", esp_err_to_name(ret));
        }
        bus_initialized_ = false;
    }
    
    // SPIバス設定構造体を作成
    spi_bus_config_t bus_cfg = {};
    bus_cfg.mosi_io_num = config_.mosi_pin;
    bus_cfg.miso_io_num = config_.miso_pin;
    bus_cfg.sclk_io_num = config_.sclk_pin;
    bus_cfg.quadwp_io_num = -1;  // 使用しない
    bus_cfg.quadhd_io_num = -1;  // 使用しない
    bus_cfg.max_transfer_sz = config_.max_transfer_size;
    
    // SPIバスを初期化
    esp_err_t ret = spi_bus_initialize(config_.host, &bus_cfg, config_.dma_channel);
    if (ret != ESP_OK) {
        logError("SPIバス初期化失敗: %s", esp_err_to_name(ret));
        setState(State::ERROR);
        return ret;
    }
    
    bus_initialized_ = true;
    
    logInfo("SPI設定完了 ホスト:%d MOSI:%d MISO:%d SCLK:%d", 
            static_cast<int>(config_.host), config_.mosi_pin, config_.miso_pin, config_.sclk_pin);
    
    return ESP_OK;
}

esp_err_t SpiHal::start() {
    if (!isInitialized()) {
        return ESP_ERR_INVALID_STATE;
    }
    
    if (!bus_initialized_) {
        esp_err_t ret = configure();
        if (ret != ESP_OK) {
            return ret;
        }
    }
    
    setState(State::RUNNING);
    logInfo("SPI HAL開始");
    return ESP_OK;
}

esp_err_t SpiHal::stop() {
    setState(State::SUSPENDED);
    logInfo("SPI HAL停止");
    return ESP_OK;
}

esp_err_t SpiHal::reset() {
    // 全デバイスを削除
    for (auto device : devices_) {
        spi_bus_remove_device(device);
    }
    devices_.clear();
    
    // バスを解放
    if (bus_initialized_) {
        spi_bus_free(config_.host);
        bus_initialized_ = false;
    }
    
    setState(State::INITIALIZED);
    logInfo("SPI HALリセット完了");
    return ESP_OK;
}

esp_err_t SpiHal::setConfig(const Config& config) {
    std::lock_guard<std::mutex> lock(mutex_);
    config_ = config;
    
    logDebug("SPI設定更新 ホスト:%d", static_cast<int>(config_.host));
    
    return ESP_OK;
}

esp_err_t SpiHal::addDevice(const DeviceConfig& device_config, spi_device_handle_t& device_handle) {
    if (!isRunning()) {
        logError("SPI HALが動作していません");
        return ESP_ERR_INVALID_STATE;
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    // SPIデバイス設定構造体を作成
    spi_device_interface_config_t dev_cfg = {};
    dev_cfg.command_bits = device_config.command_bits;
    dev_cfg.address_bits = device_config.address_bits;
    dev_cfg.dummy_bits = device_config.dummy_bits;
    dev_cfg.mode = spiModeToFlags(device_config.mode);
    dev_cfg.duty_cycle_pos = 128;  // 50%デューティ
    dev_cfg.cs_ena_pretrans = device_config.cs_ena_pretrans;
    dev_cfg.cs_ena_posttrans = device_config.cs_ena_posttrans;
    dev_cfg.clock_speed_hz = device_config.frequency;
    dev_cfg.spics_io_num = config_.cs_pin;
    dev_cfg.flags = device_config.flags;
    dev_cfg.queue_size = config_.queue_size;
    dev_cfg.pre_cb = nullptr;
    dev_cfg.post_cb = nullptr;
    
    // デバイスを追加
    esp_err_t ret = spi_bus_add_device(config_.host, &dev_cfg, &device_handle);
    if (ret != ESP_OK) {
        logError("SPIデバイス追加失敗: %s", esp_err_to_name(ret));
        return ret;
    }
    
    devices_.push_back(device_handle);
    
    logInfo("SPIデバイス追加成功 周波数:%dHz モード:%d", 
            device_config.frequency, static_cast<int>(device_config.mode));
    
    return ESP_OK;
}

esp_err_t SpiHal::removeDevice(spi_device_handle_t device_handle) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // デバイスリストから削除
    auto it = std::find(devices_.begin(), devices_.end(), device_handle);
    if (it != devices_.end()) {
        devices_.erase(it);
    }
    
    // SPIバスからデバイスを削除
    esp_err_t ret = spi_bus_remove_device(device_handle);
    if (ret != ESP_OK) {
        logError("SPIデバイス削除失敗: %s", esp_err_to_name(ret));
        return ret;
    }
    
    logInfo("SPIデバイス削除成功");
    return ESP_OK;
}

esp_err_t SpiHal::transmit(spi_device_handle_t device_handle, Transaction& transaction) {
    if (!isRunning()) {
        logError("SPI HALが動作していません");
        return ESP_ERR_INVALID_STATE;
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    // トランザクション構造体を準備
    spi_transaction_t spi_trans = {};
    spi_trans.flags = transaction.flags;
    spi_trans.cmd = transaction.command;
    spi_trans.addr = transaction.address;
    spi_trans.length = transaction.length ? transaction.length : (transaction.tx_data.size() * 8);
    spi_trans.rxlength = transaction.rx_data.size() * 8;
    
    // 送信データの設定
    if (transaction.tx_data.size() <= 4) {
        // 小さいデータは内部バッファを使用
        if (!transaction.tx_data.empty()) {
            memcpy(spi_trans.tx_data, transaction.tx_data.data(), transaction.tx_data.size());
            spi_trans.flags |= SPI_TRANS_USE_TXDATA;
        }
    } else {
        // 大きいデータはポインタを使用
        spi_trans.tx_buffer = transaction.tx_data.data();
    }
    
    // 受信バッファの設定
    if (!transaction.rx_data.empty()) {
        if (transaction.rx_data.size() <= 4) {
            spi_trans.flags |= SPI_TRANS_USE_RXDATA;
        } else {
            spi_trans.rx_buffer = transaction.rx_data.data();
        }
    }
    
    // トランザクション実行
    esp_err_t ret = spi_device_polling_transmit(device_handle, &spi_trans);
    if (ret != ESP_OK) {
        logError("SPIトランザクション失敗: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // 小さい受信データをコピー
    if ((spi_trans.flags & SPI_TRANS_USE_RXDATA) && !transaction.rx_data.empty()) {
        memcpy(transaction.rx_data.data(), spi_trans.rx_data, 
               std::min(transaction.rx_data.size(), size_t(4)));
    }
    
    logDebug("SPIトランザクション成功 送信:%zuバイト 受信:%zuバイト", 
             transaction.tx_data.size(), transaction.rx_data.size());
    
    return ESP_OK;
}

esp_err_t SpiHal::write(spi_device_handle_t device_handle, const std::vector<uint8_t>& data) {
    Transaction trans;
    trans.tx_data = data;
    trans.length = data.size() * 8;
    
    return transmit(device_handle, trans);
}

esp_err_t SpiHal::read(spi_device_handle_t device_handle, std::vector<uint8_t>& data, size_t length) {
    Transaction trans;
    trans.rx_data.resize(length);
    trans.length = length * 8;
    trans.rxlength = length * 8;
    
    esp_err_t ret = transmit(device_handle, trans);
    if (ret == ESP_OK) {
        data = std::move(trans.rx_data);
    }
    
    return ret;
}

esp_err_t SpiHal::writeRegister(spi_device_handle_t device_handle, uint8_t address, 
                               const std::vector<uint8_t>& data) {
    Transaction trans;
    trans.tx_data.push_back(address & 0x7F);  // 書き込みビット（MSB=0）
    trans.tx_data.insert(trans.tx_data.end(), data.begin(), data.end());
    trans.length = trans.tx_data.size() * 8;
    
    return transmit(device_handle, trans);
}

esp_err_t SpiHal::readRegister(spi_device_handle_t device_handle, uint8_t address, 
                              std::vector<uint8_t>& data, size_t length) {
    Transaction trans;
    trans.tx_data.push_back(address | 0x80);  // 読み取りビット（MSB=1）
    
    // ダミーバイトを追加（読み取り用）
    trans.tx_data.resize(length + 1, 0);
    trans.rx_data.resize(length + 1);
    trans.length = trans.tx_data.size() * 8;
    trans.rxlength = trans.rx_data.size() * 8;
    
    esp_err_t ret = transmit(device_handle, trans);
    if (ret == ESP_OK) {
        // 最初のバイト（アドレスエコー）をスキップ
        data.assign(trans.rx_data.begin() + 1, trans.rx_data.end());
    }
    
    return ret;
}

esp_err_t SpiHal::writeRegister8(spi_device_handle_t device_handle, uint8_t address, uint8_t value) {
    std::vector<uint8_t> data = {value};
    return writeRegister(device_handle, address, data);
}

esp_err_t SpiHal::readRegister8(spi_device_handle_t device_handle, uint8_t address, uint8_t& value) {
    std::vector<uint8_t> data;
    esp_err_t ret = readRegister(device_handle, address, data, 1);
    if (ret == ESP_OK && !data.empty()) {
        value = data[0];
    }
    return ret;
}

uint32_t SpiHal::spiModeToFlags(SpiMode mode) {
    switch (mode) {
        case SpiMode::MODE0:  // CPOL=0, CPHA=0
            return 0;
        case SpiMode::MODE1:  // CPOL=0, CPHA=1
            return SPI_DEVICE_POSITIVE_CS;
        case SpiMode::MODE2:  // CPOL=1, CPHA=0
            return SPI_DEVICE_CLK_AS_CS;
        case SpiMode::MODE3:  // CPOL=1, CPHA=1
            return SPI_DEVICE_POSITIVE_CS | SPI_DEVICE_CLK_AS_CS;
        default:
            return 0;
    }
}

} // namespace hal