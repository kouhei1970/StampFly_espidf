/*
 * ADC HAL Class Implementation
 * 
 * ADC（アナログ・デジタル変換）用のハードウェア抽象化レイヤー実装
 * 
 * 作成者: Kouhei Ito
 * ライセンス: MIT License
 * 
 * Copyright (c) 2025 Kouhei Ito
 */

#include "adc_hal.hpp"
#include "esp_log.h"
#include <algorithm>
#include <numeric>

namespace hal {

AdcHal::AdcHal(Unit unit) 
    : HalBase("ADC_HAL")
    , adc_handle_(nullptr)
    , nvs_initialized_(false) {
    config_.unit = unit;
    config_.bit_width = BitWidth::WIDTH_DEFAULT;
    config_.default_vref = 1100; // デフォルト1100mV
    
    logDebug("ADC HALクラス作成 ユニット:%d", static_cast<int>(unit));
}

AdcHal::~AdcHal() {
    // キャリブレーションハンドルを解放
    for (auto& pair : calibration_handles_) {
        if (pair.second) {
            adc_cali_delete_scheme_curve_fitting(pair.second);
        }
    }
    calibration_handles_.clear();
    
    // ADCハンドルを解放
    if (adc_handle_) {
        adc_oneshot_del_unit(adc_handle_);
        logDebug("ADCユニット削除 ユニット:%d", static_cast<int>(config_.unit));
    }
    
    logDebug("ADC HALクラス破棄");
}

esp_err_t AdcHal::initialize() {
    setState(State::INITIALIZING);
    
    // ADCユニット初期化設定
    adc_oneshot_unit_init_cfg_t init_config = {};
    init_config.unit_id = static_cast<adc_unit_t>(config_.unit);
    init_config.clk_src = ADC_RTC_CLK_SRC_DEFAULT;
    init_config.ulp_mode = ADC_ULP_MODE_DISABLE;
    
    // ADCユニットを初期化
    esp_err_t ret = adc_oneshot_new_unit(&init_config, &adc_handle_);
    if (ret != ESP_OK) {
        logError("ADCユニット初期化失敗: %s", esp_err_to_name(ret));
        setState(State::ERROR);
        return ret;
    }
    
    setState(State::INITIALIZED);
    logInfo("ADC HAL初期化完了 ユニット:%d", static_cast<int>(config_.unit));
    return ESP_OK;
}

esp_err_t AdcHal::configure() {
    if (!isInitialized()) {
        logError("ADC HALが初期化されていません");
        return ESP_ERR_INVALID_STATE;
    }
    
    // 設定済みのチャンネルを再設定
    for (const auto& pair : channels_) {
        esp_err_t ret = configureChannel(pair.second);
        if (ret != ESP_OK) {
            logError("チャンネル%d設定失敗: %s", pair.first, esp_err_to_name(ret));
            return ret;
        }
    }
    
    logInfo("ADC HAL設定完了");
    return ESP_OK;
}

esp_err_t AdcHal::start() {
    if (!isInitialized()) {
        return ESP_ERR_INVALID_STATE;
    }
    
    setState(State::RUNNING);
    logInfo("ADC HAL開始");
    return ESP_OK;
}

esp_err_t AdcHal::stop() {
    setState(State::SUSPENDED);
    logInfo("ADC HAL停止");
    return ESP_OK;
}

esp_err_t AdcHal::reset() {
    // フィルタ値をクリア
    filter_values_.clear();
    
    setState(State::INITIALIZED);
    logInfo("ADC HALリセット完了");
    return ESP_OK;
}

esp_err_t AdcHal::setConfig(const Config& config) {
    std::lock_guard<std::mutex> lock(mutex_);
    config_ = config;
    
    logDebug("ADC設定更新 ユニット:%d ビット幅:%d", 
             static_cast<int>(config_.unit), static_cast<int>(config_.bit_width));
    
    return ESP_OK;
}

esp_err_t AdcHal::configureChannel(const ChannelConfig& channel_config) {
    if (!isInitialized()) {
        logError("ADC HALが初期化されていません");
        return ESP_ERR_INVALID_STATE;
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    // チャンネル設定
    adc_oneshot_chan_cfg_t config = {};
    config.atten = static_cast<adc_atten_t>(channel_config.attenuation);
    config.bitwidth = static_cast<adc_bitwidth_t>(config_.bit_width);
    
    esp_err_t ret = adc_oneshot_config_channel(adc_handle_, channel_config.channel, &config);
    if (ret != ESP_OK) {
        logError("チャンネル設定失敗 チャンネル:%d: %s", 
                 channel_config.channel, esp_err_to_name(ret));
        return ret;
    }
    
    // 設定を保存
    channels_[channel_config.channel] = channel_config;
    
    // キャリブレーション設定
    if (channel_config.calibration_enable) {
        ret = createCalibrationHandle(channel_config.channel, channel_config.attenuation);
        if (ret != ESP_OK) {
            logWarning("キャリブレーション設定失敗 チャンネル:%d: %s", 
                      channel_config.channel, esp_err_to_name(ret));
        }
    }
    
    logDebug("ADCチャンネル設定完了 チャンネル:%d 減衰:%d", 
             channel_config.channel, static_cast<int>(channel_config.attenuation));
    
    return ESP_OK;
}

esp_err_t AdcHal::read(adc_channel_t channel, ReadResult& result) {
    if (!isRunning()) {
        logError("ADC HALが動作していません");
        return ESP_ERR_INVALID_STATE;
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    // 生のADC値を読み取り
    esp_err_t ret = adc_oneshot_read(adc_handle_, channel, &result.raw_value);
    if (ret != ESP_OK) {
        logError("ADC読み取り失敗 チャンネル:%d: %s", channel, esp_err_to_name(ret));
        return ret;
    }
    
    // 電圧値に変換
    result.calibrated = false;
    auto cal_it = calibration_handles_.find(channel);
    if (cal_it != calibration_handles_.end() && cal_it->second) {
        ret = adc_cali_raw_to_voltage(cal_it->second, result.raw_value, &result.voltage_mv);
        if (ret == ESP_OK) {
            result.calibrated = true;
        } else {
            // キャリブレーション失敗時は推定値を使用
            result.voltage_mv = (result.raw_value * config_.default_vref) / 4095;
        }
    } else {
        // キャリブレーションなしの場合は推定値
        result.voltage_mv = (result.raw_value * config_.default_vref) / 4095;
    }
    
    logDebug("ADC読み取り チャンネル:%d 生値:%d 電圧:%dmV キャリブレーション:%s",
             channel, result.raw_value, result.voltage_mv, 
             result.calibrated ? "有効" : "無効");
    
    return ESP_OK;
}

esp_err_t AdcHal::readRaw(adc_channel_t channel, int& raw_value) {
    ReadResult result;
    esp_err_t ret = read(channel, result);
    if (ret == ESP_OK) {
        raw_value = result.raw_value;
    }
    return ret;
}

esp_err_t AdcHal::readVoltage(adc_channel_t channel, int& voltage_mv) {
    ReadResult result;
    esp_err_t ret = read(channel, result);
    if (ret == ESP_OK) {
        voltage_mv = result.voltage_mv;
    }
    return ret;
}

esp_err_t AdcHal::readAverage(adc_channel_t channel, size_t samples, ReadResult& result) {
    if (!isRunning()) {
        logError("ADC HALが動作していません");
        return ESP_ERR_INVALID_STATE;
    }
    
    if (samples == 0) {
        logError("サンプル数が0です");
        return ESP_ERR_INVALID_ARG;
    }
    
    std::vector<int> raw_values;
    raw_values.reserve(samples);
    
    // 複数サンプル取得
    for (size_t i = 0; i < samples; i++) {
        int raw_value;
        esp_err_t ret = adc_oneshot_read(adc_handle_, channel, &raw_value);
        if (ret != ESP_OK) {
            logError("ADC読み取り失敗 サンプル:%zu/%zu", i, samples);
            return ret;
        }
        raw_values.push_back(raw_value);
        
        // サンプル間の短い遅延
        vTaskDelay(1 / portTICK_PERIOD_MS);
    }
    
    // 平均値計算
    result.raw_value = std::accumulate(raw_values.begin(), raw_values.end(), 0) / samples;
    
    // 電圧値に変換
    result.calibrated = false;
    auto cal_it = calibration_handles_.find(channel);
    if (cal_it != calibration_handles_.end() && cal_it->second) {
        esp_err_t ret = adc_cali_raw_to_voltage(cal_it->second, result.raw_value, &result.voltage_mv);
        if (ret == ESP_OK) {
            result.calibrated = true;
        } else {
            result.voltage_mv = (result.raw_value * config_.default_vref) / 4095;
        }
    } else {
        result.voltage_mv = (result.raw_value * config_.default_vref) / 4095;
    }
    
    logDebug("ADC平均読み取り チャンネル:%d サンプル数:%zu 平均値:%d 電圧:%dmV",
             channel, samples, result.raw_value, result.voltage_mv);
    
    return ESP_OK;
}

esp_err_t AdcHal::readFiltered(adc_channel_t channel, float alpha, ReadResult& result) {
    if (!isRunning()) {
        logError("ADC HALが動作していません");
        return ESP_ERR_INVALID_STATE;
    }
    
    if (alpha < 0.0f || alpha > 1.0f) {
        logError("無効なフィルタ係数: %f", alpha);
        return ESP_ERR_INVALID_ARG;
    }
    
    // 現在の値を読み取り
    esp_err_t ret = read(channel, result);
    if (ret != ESP_OK) {
        return ret;
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    // 指数移動平均フィルタ
    auto it = filter_values_.find(channel);
    if (it != filter_values_.end()) {
        // フィルタ値更新: new_value = alpha * current + (1 - alpha) * previous
        float filtered = alpha * result.raw_value + (1.0f - alpha) * it->second;
        it->second = filtered;
        result.raw_value = static_cast<int>(filtered);
        
        // フィルタ後の電圧値を再計算
        auto cal_it = calibration_handles_.find(channel);
        if (cal_it != calibration_handles_.end() && cal_it->second) {
            adc_cali_raw_to_voltage(cal_it->second, result.raw_value, &result.voltage_mv);
        } else {
            result.voltage_mv = (result.raw_value * config_.default_vref) / 4095;
        }
    } else {
        // 初回はフィルタなし
        filter_values_[channel] = static_cast<float>(result.raw_value);
    }
    
    return ESP_OK;
}

esp_err_t AdcHal::setAttenuation(adc_channel_t channel, Attenuation attenuation) {
    auto it = channels_.find(channel);
    if (it == channels_.end()) {
        logError("未設定のチャンネル: %d", channel);
        return ESP_ERR_INVALID_ARG;
    }
    
    // 設定を更新
    it->second.attenuation = attenuation;
    
    // チャンネルを再設定
    return configureChannel(it->second);
}

esp_err_t AdcHal::setBitWidth(BitWidth bit_width) {
    config_.bit_width = bit_width;
    
    // 全チャンネルを再設定
    return configure();
}

esp_err_t AdcHal::calibrate(adc_channel_t channel) {
    auto it = channels_.find(channel);
    if (it == channels_.end()) {
        logError("未設定のチャンネル: %d", channel);
        return ESP_ERR_INVALID_ARG;
    }
    
    return createCalibrationHandle(channel, it->second.attenuation);
}

esp_err_t AdcHal::calibrateAll() {
    esp_err_t ret = ESP_OK;
    
    for (const auto& pair : channels_) {
        esp_err_t cal_ret = createCalibrationHandle(pair.first, pair.second.attenuation);
        if (cal_ret != ESP_OK) {
            ret = cal_ret;
            logWarning("チャンネル%dキャリブレーション失敗", pair.first);
        }
    }
    
    return ret;
}

esp_err_t AdcHal::convertToVoltage(adc_channel_t channel, int raw_value, int& voltage_mv) {
    auto cal_it = calibration_handles_.find(channel);
    if (cal_it != calibration_handles_.end() && cal_it->second) {
        return adc_cali_raw_to_voltage(cal_it->second, raw_value, &voltage_mv);
    } else {
        // キャリブレーションなしの場合は推定値
        voltage_mv = (raw_value * config_.default_vref) / 4095;
        return ESP_OK;
    }
}

bool AdcHal::isValidChannel(adc_channel_t channel) const {
    // ESP32-S3のADCチャンネル範囲確認
    if (config_.unit == Unit::UNIT_1) {
        return channel >= ADC_CHANNEL_0 && channel <= ADC_CHANNEL_9;
    } else {
        return channel >= ADC_CHANNEL_0 && channel <= ADC_CHANNEL_9;
    }
}

esp_err_t AdcHal::createCalibrationHandle(adc_channel_t channel, Attenuation attenuation) {
    // 既存のハンドルを削除
    destroyCalibrationHandle(channel);
    
    // キャリブレーション設定
    adc_cali_curve_fitting_config_t cali_config = {};
    cali_config.unit_id = static_cast<adc_unit_t>(config_.unit);
    cali_config.chan = channel;
    cali_config.atten = static_cast<adc_atten_t>(attenuation);
    cali_config.bitwidth = static_cast<adc_bitwidth_t>(config_.bit_width);
    
    adc_cali_handle_t handle = nullptr;
    esp_err_t ret = adc_cali_create_scheme_curve_fitting(&cali_config, &handle);
    if (ret != ESP_OK) {
        logError("キャリブレーションハンドル作成失敗: %s", esp_err_to_name(ret));
        return ret;
    }
    
    calibration_handles_[channel] = handle;
    logInfo("ADCキャリブレーション設定 チャンネル:%d", channel);
    
    return ESP_OK;
}

void AdcHal::destroyCalibrationHandle(adc_channel_t channel) {
    auto it = calibration_handles_.find(channel);
    if (it != calibration_handles_.end() && it->second) {
        adc_cali_delete_scheme_curve_fitting(it->second);
        calibration_handles_.erase(it);
    }
}

} // namespace hal