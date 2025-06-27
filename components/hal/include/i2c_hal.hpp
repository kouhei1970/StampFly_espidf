/*
 * I2C HAL Class
 * 
 * I2C通信用のハードウェア抽象化レイヤー
 * ESP-IDF I2C APIをC++クラスでラップ
 * 
 * 作成者: Kouhei Ito
 * ライセンス: MIT License
 * 
 * Copyright (c) 2025 Kouhei Ito
 */

#ifndef I2C_HAL_HPP
#define I2C_HAL_HPP

#include "hal_base.hpp"
#include "driver/i2c.h"
#include <vector>
#include <memory>
#include <mutex>

namespace hal {

/**
 * @brief I2C HALクラス
 * 
 * I2C通信の抽象化レイヤー
 * ESP-IDF I2C APIのC++ラッパー
 */
class I2cHal : public HalBase {
public:
    /**
     * @brief I2Cモード列挙型
     */
    enum class Mode {
        MASTER = I2C_MODE_MASTER,
        SLAVE = I2C_MODE_SLAVE
    };

    /**
     * @brief I2C設定構造体
     */
    struct Config {
        i2c_port_t port;            // I2Cポート番号
        Mode mode;                  // マスター/スレーブモード
        gpio_num_t sda_pin;         // SDAピン
        gpio_num_t scl_pin;         // SCLピン
        uint32_t frequency;         // クロック周波数（Hz）
        bool sda_pullup_enable;     // SDAプルアップ有効
        bool scl_pullup_enable;     // SCLプルアップ有効
        uint8_t slave_address;      // スレーブアドレス（スレーブモード時のみ）
    };

    /**
     * @brief I2Cトランザクション構造体
     */
    struct Transaction {
        uint8_t device_address;     // デバイスアドレス
        uint8_t register_address;   // レジスタアドレス
        std::vector<uint8_t> data;  // データ
        bool use_register_address;  // レジスタアドレス使用フラグ
        TickType_t timeout;         // タイムアウト時間
    };

public:
    /**
     * @brief コンストラクタ
     * @param port I2Cポート番号
     */
    explicit I2cHal(i2c_port_t port = I2C_NUM_0);

    /**
     * @brief デストラクタ
     */
    virtual ~I2cHal();

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
     * @brief I2C設定
     * @param config I2C設定
     * @return esp_err_t 設定結果
     */
    esp_err_t setConfig(const Config& config);

    /**
     * @brief データ書き込み
     * @param device_address デバイスアドレス
     * @param data 書き込みデータ
     * @param timeout タイムアウト時間
     * @return esp_err_t 書き込み結果
     */
    esp_err_t write(uint8_t device_address, const std::vector<uint8_t>& data, 
                    TickType_t timeout = pdMS_TO_TICKS(1000));

    /**
     * @brief データ読み取り
     * @param device_address デバイスアドレス
     * @param data 読み取りデータ格納先
     * @param length 読み取りデータ長
     * @param timeout タイムアウト時間
     * @return esp_err_t 読み取り結果
     */
    esp_err_t read(uint8_t device_address, std::vector<uint8_t>& data, 
                   size_t length, TickType_t timeout = pdMS_TO_TICKS(1000));

    /**
     * @brief レジスタ書き込み
     * @param device_address デバイスアドレス
     * @param register_address レジスタアドレス
     * @param data 書き込みデータ
     * @param timeout タイムアウト時間
     * @return esp_err_t 書き込み結果
     */
    esp_err_t writeRegister(uint8_t device_address, uint8_t register_address, 
                           const std::vector<uint8_t>& data, 
                           TickType_t timeout = pdMS_TO_TICKS(1000));

    /**
     * @brief レジスタ読み取り
     * @param device_address デバイスアドレス
     * @param register_address レジスタアドレス
     * @param data 読み取りデータ格納先
     * @param length 読み取りデータ長
     * @param timeout タイムアウト時間
     * @return esp_err_t 読み取り結果
     */
    esp_err_t readRegister(uint8_t device_address, uint8_t register_address, 
                          std::vector<uint8_t>& data, size_t length, 
                          TickType_t timeout = pdMS_TO_TICKS(1000));

    /**
     * @brief 8bit値書き込み
     * @param device_address デバイスアドレス
     * @param register_address レジスタアドレス
     * @param value 書き込み値
     * @param timeout タイムアウト時間
     * @return esp_err_t 書き込み結果
     */
    esp_err_t writeRegister8(uint8_t device_address, uint8_t register_address, 
                            uint8_t value, TickType_t timeout = pdMS_TO_TICKS(1000));

    /**
     * @brief 8bit値読み取り
     * @param device_address デバイスアドレス
     * @param register_address レジスタアドレス
     * @param value 読み取り値格納先
     * @param timeout タイムアウト時間
     * @return esp_err_t 読み取り結果
     */
    esp_err_t readRegister8(uint8_t device_address, uint8_t register_address, 
                           uint8_t& value, TickType_t timeout = pdMS_TO_TICKS(1000));

    /**
     * @brief 16bit値書き込み
     * @param device_address デバイスアドレス
     * @param register_address レジスタアドレス
     * @param value 書き込み値
     * @param big_endian ビッグエンディアン指定
     * @param timeout タイムアウト時間
     * @return esp_err_t 書き込み結果
     */
    esp_err_t writeRegister16(uint8_t device_address, uint8_t register_address, 
                             uint16_t value, bool big_endian = true, 
                             TickType_t timeout = pdMS_TO_TICKS(1000));

    /**
     * @brief 16bit値読み取り
     * @param device_address デバイスアドレス
     * @param register_address レジスタアドレス
     * @param value 読み取り値格納先
     * @param big_endian ビッグエンディアン指定
     * @param timeout タイムアウト時間
     * @return esp_err_t 読み取り結果
     */
    esp_err_t readRegister16(uint8_t device_address, uint8_t register_address, 
                            uint16_t& value, bool big_endian = true, 
                            TickType_t timeout = pdMS_TO_TICKS(1000));

    /**
     * @brief デバイス存在確認
     * @param device_address デバイスアドレス
     * @param timeout タイムアウト時間
     * @return bool デバイスが存在する場合true
     */
    bool deviceExists(uint8_t device_address, TickType_t timeout = pdMS_TO_TICKS(100));

    /**
     * @brief バススキャン
     * @param found_devices 発見されたデバイスアドレス格納先
     * @return esp_err_t スキャン結果
     */
    esp_err_t scanBus(std::vector<uint8_t>& found_devices);

    /**
     * @brief I2Cポート番号取得
     * @return i2c_port_t I2Cポート番号
     */
    i2c_port_t getPort() const { return config_.port; }

private:
    Config config_;                 // I2C設定
    std::mutex mutex_;              // スレッドセーフ用ミューテックス
    bool driver_installed_;         // ドライバインストール状態

    /**
     * @brief トランザクション実行
     * @param transaction トランザクション情報
     * @param is_read 読み取りトランザクションの場合true
     * @return esp_err_t 実行結果
     */
    esp_err_t executeTransaction(const Transaction& transaction, bool is_read);
};

} // namespace hal

#endif // I2C_HAL_HPP