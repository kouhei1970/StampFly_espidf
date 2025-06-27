/*
 * ADC HAL Class
 * 
 * ADC（アナログ・デジタル変換）用のハードウェア抽象化レイヤー
 * ESP-IDF ADC APIをC++クラスでラップ
 * 
 * 作成者: Kouhei Ito
 * ライセンス: MIT License
 * 
 * Copyright (c) 2025 Kouhei Ito
 */

#ifndef ADC_HAL_HPP
#define ADC_HAL_HPP

#include "hal_base.hpp"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include <memory>
#include <mutex>
#include <map>

namespace hal {

/**
 * @brief ADC HALクラス
 * 
 * ADC操作の抽象化レイヤー
 * ESP-IDF ADC APIのC++ラッパー
 */
class AdcHal : public HalBase {
public:
    /**
     * @brief ADCユニット列挙型
     */
    enum class Unit {
        UNIT_1 = ADC_UNIT_1,
        UNIT_2 = ADC_UNIT_2
    };

    /**
     * @brief ADC減衰列挙型
     */
    enum class Attenuation {
        DB_0 = ADC_ATTEN_DB_0,      // 0dB (100mV ~ 950mV)
        DB_2_5 = ADC_ATTEN_DB_2_5,  // 2.5dB (100mV ~ 1250mV)
        DB_6 = ADC_ATTEN_DB_6,      // 6dB (150mV ~ 1750mV)
        DB_11 = ADC_ATTEN_DB_11     // 11dB (150mV ~ 2450mV)
    };

    /**
     * @brief ADCビット幅列挙型
     */
    enum class BitWidth {
        WIDTH_9BIT = ADC_BITWIDTH_9,
        WIDTH_10BIT = ADC_BITWIDTH_10,
        WIDTH_11BIT = ADC_BITWIDTH_11,
        WIDTH_12BIT = ADC_BITWIDTH_12,
        WIDTH_13BIT = ADC_BITWIDTH_13,
        WIDTH_DEFAULT = ADC_BITWIDTH_DEFAULT
    };

    /**
     * @brief ADCチャンネル設定構造体
     */
    struct ChannelConfig {
        adc_channel_t channel;      // チャンネル番号
        Attenuation attenuation;    // 減衰設定
        bool calibration_enable;    // キャリブレーション有効化
    };

    /**
     * @brief ADC設定構造体
     */
    struct Config {
        Unit unit;                  // ADCユニット
        BitWidth bit_width;         // ビット幅
        uint32_t default_vref;      // デフォルトVref値（mV）
    };

    /**
     * @brief ADC読み取り結果構造体
     */
    struct ReadResult {
        int raw_value;              // 生のADC値
        int voltage_mv;             // 電圧値（mV）
        bool calibrated;            // キャリブレーション済みフラグ
    };

public:
    /**
     * @brief コンストラクタ
     * @param unit ADCユニット
     */
    explicit AdcHal(Unit unit = Unit::UNIT_1);

    /**
     * @brief デストラクタ
     */
    virtual ~AdcHal();

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
     * @brief ADC設定
     * @param config ADC設定
     * @return esp_err_t 設定結果
     */
    esp_err_t setConfig(const Config& config);

    /**
     * @brief チャンネル設定
     * @param channel_config チャンネル設定
     * @return esp_err_t 設定結果
     */
    esp_err_t configureChannel(const ChannelConfig& channel_config);

    /**
     * @brief 単一読み取り
     * @param channel チャンネル番号
     * @param result 読み取り結果格納先
     * @return esp_err_t 読み取り結果
     */
    esp_err_t read(adc_channel_t channel, ReadResult& result);

    /**
     * @brief 生のADC値読み取り
     * @param channel チャンネル番号
     * @param raw_value 生のADC値格納先
     * @return esp_err_t 読み取り結果
     */
    esp_err_t readRaw(adc_channel_t channel, int& raw_value);

    /**
     * @brief 電圧値読み取り
     * @param channel チャンネル番号
     * @param voltage_mv 電圧値格納先（mV）
     * @return esp_err_t 読み取り結果
     */
    esp_err_t readVoltage(adc_channel_t channel, int& voltage_mv);

    /**
     * @brief 複数サンプル平均読み取り
     * @param channel チャンネル番号
     * @param samples サンプル数
     * @param result 平均結果格納先
     * @return esp_err_t 読み取り結果
     */
    esp_err_t readAverage(adc_channel_t channel, size_t samples, ReadResult& result);

    /**
     * @brief フィルタ付き読み取り
     * @param channel チャンネル番号
     * @param alpha フィルタ係数（0.0-1.0）
     * @param result フィルタ結果格納先
     * @return esp_err_t 読み取り結果
     */
    esp_err_t readFiltered(adc_channel_t channel, float alpha, ReadResult& result);

    /**
     * @brief チャンネル減衰設定
     * @param channel チャンネル番号
     * @param attenuation 減衰設定
     * @return esp_err_t 設定結果
     */
    esp_err_t setAttenuation(adc_channel_t channel, Attenuation attenuation);

    /**
     * @brief ビット幅設定
     * @param bit_width ビット幅
     * @return esp_err_t 設定結果
     */
    esp_err_t setBitWidth(BitWidth bit_width);

    /**
     * @brief キャリブレーション実行
     * @param channel チャンネル番号
     * @return esp_err_t キャリブレーション結果
     */
    esp_err_t calibrate(adc_channel_t channel);

    /**
     * @brief 全チャンネルキャリブレーション
     * @return esp_err_t キャリブレーション結果
     */
    esp_err_t calibrateAll();

    /**
     * @brief ADC値から電圧値変換
     * @param channel チャンネル番号
     * @param raw_value 生のADC値
     * @param voltage_mv 電圧値格納先（mV）
     * @return esp_err_t 変換結果
     */
    esp_err_t convertToVoltage(adc_channel_t channel, int raw_value, int& voltage_mv);

    /**
     * @brief チャンネルの有効性確認
     * @param channel チャンネル番号
     * @return bool 有効なチャンネルの場合true
     */
    bool isValidChannel(adc_channel_t channel) const;

    /**
     * @brief ADCユニット取得
     * @return Unit ADCユニット
     */
    Unit getUnit() const { return config_.unit; }

private:
    Config config_;                                     // ADC設定
    std::mutex mutex_;                                  // スレッドセーフ用ミューテックス
    adc_oneshot_unit_handle_t adc_handle_;             // ADCハンドル
    std::map<adc_channel_t, ChannelConfig> channels_; // チャンネル設定管理
    std::map<adc_channel_t, adc_cali_handle_t> calibration_handles_; // キャリブレーションハンドル
    std::map<adc_channel_t, float> filter_values_;    // フィルタ値管理
    
    /**
     * @brief キャリブレーションハンドル作成
     * @param channel チャンネル番号
     * @param attenuation 減衰設定
     * @return esp_err_t 作成結果
     */
    esp_err_t createCalibrationHandle(adc_channel_t channel, Attenuation attenuation);
    
    /**
     * @brief キャリブレーションハンドル削除
     * @param channel チャンネル番号
     */
    void destroyCalibrationHandle(adc_channel_t channel);
};

} // namespace hal

#endif // ADC_HAL_HPP