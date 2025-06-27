/*
 * GPIO HAL Class
 * 
 * GPIO操作用のハードウェア抽象化レイヤー
 * ESP-IDF GPIO APIをC++クラスでラップ
 * 
 * 作成者: Kouhei Ito
 * ライセンス: MIT License
 * 
 * Copyright (c) 2025 Kouhei Ito
 */

#ifndef GPIO_HAL_HPP
#define GPIO_HAL_HPP

#include "hal_base.hpp"
#include "driver/gpio.h"
#include <functional>
#include <map>

namespace hal {

/**
 * @brief GPIO HALクラス
 * 
 * GPIO操作の抽象化レイヤー
 * ESP-IDF GPIO APIのC++ラッパー
 */
class GpioHal : public HalBase {
public:
    /**
     * @brief GPIO方向列挙型
     */
    enum class Direction {
        INPUT = GPIO_MODE_INPUT,
        OUTPUT = GPIO_MODE_OUTPUT,
        INPUT_OUTPUT = GPIO_MODE_INPUT_OUTPUT
    };

    /**
     * @brief GPIO プルアップ/プルダウン設定
     */
    enum class Pull {
        NONE = GPIO_FLOATING,
        PULLUP = GPIO_PULLUP_ONLY,
        PULLDOWN = GPIO_PULLDOWN_ONLY,
        PULLUP_PULLDOWN = GPIO_PULLUP_PULLDOWN
    };

    /**
     * @brief GPIO割り込みタイプ
     */
    enum class InterruptType {
        DISABLE = GPIO_INTR_DISABLE,
        POSEDGE = GPIO_INTR_POSEDGE,
        NEGEDGE = GPIO_INTR_NEGEDGE,
        ANYEDGE = GPIO_INTR_ANYEDGE,
        LOW_LEVEL = GPIO_INTR_LOW_LEVEL,
        HIGH_LEVEL = GPIO_INTR_HIGH_LEVEL
    };

    /**
     * @brief GPIO設定構造体
     */
    struct Config {
        gpio_num_t pin;             // ピン番号
        Direction direction;        // 入出力方向
        Pull pull;                  // プルアップ/プルダウン
        InterruptType interrupt;    // 割り込み設定
        bool invert;               // 論理反転
    };

    /**
     * @brief 割り込みコールバック関数型
     */
    using InterruptCallback = std::function<void(gpio_num_t pin, bool level)>;

private:
    static std::map<gpio_num_t, GpioHal*> pin_map_;  // ピン番号とインスタンスの対応
    static bool isr_service_installed_;              // ISRサービス初期化フラグ

public:
    /**
     * @brief コンストラクタ
     */
    GpioHal();

    /**
     * @brief デストラクタ
     */
    virtual ~GpioHal();

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
     * @brief ピン設定
     * @param config ピン設定
     * @return esp_err_t 設定結果
     */
    esp_err_t configurePin(const Config& config);

    /**
     * @brief デジタル出力
     * @param pin ピン番号
     * @param level 出力レベル（true=HIGH, false=LOW）
     * @return esp_err_t 出力結果
     */
    esp_err_t digitalWrite(gpio_num_t pin, bool level);

    /**
     * @brief デジタル入力
     * @param pin ピン番号
     * @param level 入力レベル格納先
     * @return esp_err_t 読み取り結果
     */
    esp_err_t digitalRead(gpio_num_t pin, bool& level);

    /**
     * @brief ピン方向設定
     * @param pin ピン番号
     * @param direction 入出力方向
     * @return esp_err_t 設定結果
     */
    esp_err_t setDirection(gpio_num_t pin, Direction direction);

    /**
     * @brief プルアップ/プルダウン設定
     * @param pin ピン番号
     * @param pull プル設定
     * @return esp_err_t 設定結果
     */
    esp_err_t setPull(gpio_num_t pin, Pull pull);

    /**
     * @brief 割り込み設定
     * @param pin ピン番号
     * @param type 割り込みタイプ
     * @param callback コールバック関数
     * @return esp_err_t 設定結果
     */
    esp_err_t setInterrupt(gpio_num_t pin, InterruptType type, InterruptCallback callback);

    /**
     * @brief 割り込み無効化
     * @param pin ピン番号
     * @return esp_err_t 無効化結果
     */
    esp_err_t disableInterrupt(gpio_num_t pin);

    /**
     * @brief ピンの有効性確認
     * @param pin ピン番号
     * @return bool 有効なピンの場合true
     */
    static bool isValidPin(gpio_num_t pin);

private:
    std::map<gpio_num_t, Config> pin_configs_;      // ピン設定管理
    std::map<gpio_num_t, InterruptCallback> callbacks_;  // コールバック管理

    /**
     * @brief ISRサービス初期化
     * @return esp_err_t 初期化結果
     */
    static esp_err_t installIsrService();

    /**
     * @brief GPIO割り込みハンドラ
     * @param arg 引数（未使用）
     */
    static void IRAM_ATTR gpioIsrHandler(void* arg);
};

} // namespace hal

#endif // GPIO_HAL_HPP