/*
 * SPI HAL Class
 * 
 * SPI通信用のハードウェア抽象化レイヤー
 * ESP-IDF SPI APIをC++クラスでラップ
 * 
 * 作成者: Kouhei Ito
 * ライセンス: MIT License
 * 
 * Copyright (c) 2025 Kouhei Ito
 */

#ifndef SPI_HAL_HPP
#define SPI_HAL_HPP

#include "hal_base.hpp"
#include "driver/spi_master.h"
#include <vector>
#include <memory>
#include <mutex>

namespace hal {

/**
 * @brief SPI HALクラス
 * 
 * SPI通信の抽象化レイヤー
 * ESP-IDF SPI APIのC++ラッパー
 */
class SpiHal : public HalBase {
public:
    /**
     * @brief SPIモード列挙型
     */
    enum class SpiMode {
        MODE0 = 0,  // CPOL=0, CPHA=0
        MODE1 = 1,  // CPOL=0, CPHA=1
        MODE2 = 2,  // CPOL=1, CPHA=0
        MODE3 = 3   // CPOL=1, CPHA=1
    };

    /**
     * @brief SPI設定構造体
     */
    struct Config {
        spi_host_device_t host;     // SPIホスト
        gpio_num_t mosi_pin;        // MOSIピン
        gpio_num_t miso_pin;        // MISOピン
        gpio_num_t sclk_pin;        // SCLKピン
        gpio_num_t cs_pin;          // CSピン（-1で使用しない）
        int max_transfer_size;      // 最大転送サイズ
        int dma_channel;            // DMAチャンネル
        int queue_size;             // キューサイズ
    };

    /**
     * @brief SPIデバイス設定構造体
     */
    struct DeviceConfig {
        uint32_t frequency;         // クロック周波数（Hz）
        SpiMode mode;              // SPIモード
        int cs_ena_pretrans;       // CS有効化前の待機サイクル
        int cs_ena_posttrans;      // CS無効化後の待機サイクル
        int command_bits;          // コマンドビット数
        int address_bits;          // アドレスビット数
        int dummy_bits;            // ダミービット数
        uint32_t flags;            // 追加フラグ
    };

    /**
     * @brief SPIトランザクション構造体
     */
    struct Transaction {
        uint16_t command;                   // コマンド
        uint64_t address;                  // アドレス
        std::vector<uint8_t> tx_data;      // 送信データ
        std::vector<uint8_t> rx_data;      // 受信データ
        size_t length;                     // データ長（ビット単位）
        uint32_t flags;                    // トランザクションフラグ
    };

public:
    /**
     * @brief コンストラクタ
     * @param host SPIホスト
     */
    explicit SpiHal(spi_host_device_t host = SPI2_HOST);

    /**
     * @brief デストラクタ
     */
    virtual ~SpiHal();

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
     * @brief SPI設定
     * @param config SPI設定
     * @return esp_err_t 設定結果
     */
    esp_err_t setConfig(const Config& config);

    /**
     * @brief SPIデバイス追加
     * @param device_config デバイス設定
     * @param device_handle デバイスハンドル格納先
     * @return esp_err_t 追加結果
     */
    esp_err_t addDevice(const DeviceConfig& device_config, spi_device_handle_t& device_handle);

    /**
     * @brief SPIデバイス削除
     * @param device_handle デバイスハンドル
     * @return esp_err_t 削除結果
     */
    esp_err_t removeDevice(spi_device_handle_t device_handle);

    /**
     * @brief データ送受信
     * @param device_handle デバイスハンドル
     * @param transaction トランザクション情報
     * @return esp_err_t 送受信結果
     */
    esp_err_t transmit(spi_device_handle_t device_handle, Transaction& transaction);

    /**
     * @brief データ送信のみ
     * @param device_handle デバイスハンドル
     * @param data 送信データ
     * @return esp_err_t 送信結果
     */
    esp_err_t write(spi_device_handle_t device_handle, const std::vector<uint8_t>& data);

    /**
     * @brief データ受信のみ
     * @param device_handle デバイスハンドル
     * @param data 受信データ格納先
     * @param length 受信データ長
     * @return esp_err_t 受信結果
     */
    esp_err_t read(spi_device_handle_t device_handle, std::vector<uint8_t>& data, size_t length);

    /**
     * @brief レジスタ書き込み
     * @param device_handle デバイスハンドル
     * @param address レジスタアドレス
     * @param data 書き込みデータ
     * @return esp_err_t 書き込み結果
     */
    esp_err_t writeRegister(spi_device_handle_t device_handle, uint8_t address, 
                           const std::vector<uint8_t>& data);

    /**
     * @brief レジスタ読み取り
     * @param device_handle デバイスハンドル
     * @param address レジスタアドレス
     * @param data 読み取りデータ格納先
     * @param length 読み取りデータ長
     * @return esp_err_t 読み取り結果
     */
    esp_err_t readRegister(spi_device_handle_t device_handle, uint8_t address, 
                          std::vector<uint8_t>& data, size_t length);

    /**
     * @brief 8bit値書き込み
     * @param device_handle デバイスハンドル
     * @param address レジスタアドレス
     * @param value 書き込み値
     * @return esp_err_t 書き込み結果
     */
    esp_err_t writeRegister8(spi_device_handle_t device_handle, uint8_t address, uint8_t value);

    /**
     * @brief 8bit値読み取り
     * @param device_handle デバイスハンドル
     * @param address レジスタアドレス
     * @param value 読み取り値格納先
     * @return esp_err_t 読み取り結果
     */
    esp_err_t readRegister8(spi_device_handle_t device_handle, uint8_t address, uint8_t& value);

    /**
     * @brief SPIホスト取得
     * @return spi_host_device_t SPIホスト
     */
    spi_host_device_t getHost() const { return config_.host; }

private:
    Config config_;                     // SPI設定
    std::mutex mutex_;                  // スレッドセーフ用ミューテックス
    bool bus_initialized_;              // バス初期化状態
    std::vector<spi_device_handle_t> devices_;  // デバイスハンドル管理

    /**
     * @brief SPIモードをESP-IDFフラグに変換
     * @param mode SPIモード
     * @return uint32_t ESP-IDFフラグ
     */
    uint32_t spiModeToFlags(SpiMode mode);
};

} // namespace hal

#endif // SPI_HAL_HPP