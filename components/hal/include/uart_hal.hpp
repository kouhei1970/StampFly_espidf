/*
 * UART HAL Class
 * 
 * UART通信用のハードウェア抽象化レイヤー
 * ESP-IDF UART APIをC++クラスでラップ
 * 
 * 作成者: Kouhei Ito
 * ライセンス: MIT License
 * 
 * Copyright (c) 2025 Kouhei Ito
 */

#ifndef UART_HAL_HPP
#define UART_HAL_HPP

#include "hal_base.hpp"
#include "driver/uart.h"
#include <vector>
#include <string>
#include <memory>
#include <mutex>
#include <functional>

namespace hal {

/**
 * @brief UART HALクラス
 * 
 * UART通信の抽象化レイヤー
 * ESP-IDF UART APIのC++ラッパー
 */
class UartHal : public HalBase {
public:
    /**
     * @brief UARTパリティ列挙型
     */
    enum class Parity {
        NONE = UART_PARITY_DISABLE,
        EVEN = UART_PARITY_EVEN,
        ODD = UART_PARITY_ODD
    };

    /**
     * @brief UARTストップビット列挙型
     */
    enum class StopBits {
        BITS_1 = UART_STOP_BITS_1,
        BITS_1_5 = UART_STOP_BITS_1_5,
        BITS_2 = UART_STOP_BITS_2
    };

    /**
     * @brief UARTフロー制御列挙型
     */
    enum class FlowControl {
        NONE = UART_HW_FLOWCTRL_DISABLE,
        RTS = UART_HW_FLOWCTRL_RTS,
        CTS = UART_HW_FLOWCTRL_CTS,
        RTS_CTS = UART_HW_FLOWCTRL_CTS_RTS
    };

    /**
     * @brief UART設定構造体
     */
    struct Config {
        uart_port_t port;           // UARTポート番号
        uint32_t baudrate;          // ボーレート
        uart_word_length_t data_bits; // データビット数
        Parity parity;              // パリティ
        StopBits stop_bits;         // ストップビット
        FlowControl flow_control;   // フロー制御
        gpio_num_t tx_pin;          // TXピン
        gpio_num_t rx_pin;          // RXピン
        gpio_num_t rts_pin;         // RTSピン（フロー制御時）
        gpio_num_t cts_pin;         // CTSピン（フロー制御時）
        size_t rx_buffer_size;      // 受信バッファサイズ
        size_t tx_buffer_size;      // 送信バッファサイズ
        int queue_size;             // イベントキューサイズ
    };

    /**
     * @brief UARTイベント列挙型
     */
    enum class EventType {
        DATA = UART_DATA,
        BREAK = UART_BREAK,
        BUFFER_FULL = UART_BUFFER_FULL,
        FIFO_OVERFLOW = UART_FIFO_OVF,
        FRAME_ERROR = UART_FRAME_ERR,
        PARITY_ERROR = UART_PARITY_ERR,
        DATA_BREAK = UART_DATA_BREAK,
        PATTERN_DETECTED = UART_PATTERN_DET
    };

    /**
     * @brief イベントコールバック関数型
     */
    using EventCallback = std::function<void(EventType type, size_t size)>;

public:
    /**
     * @brief コンストラクタ
     * @param port UARTポート番号
     */
    explicit UartHal(uart_port_t port = UART_NUM_0);

    /**
     * @brief デストラクタ
     */
    virtual ~UartHal();

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
     * @brief UART設定
     * @param config UART設定
     * @return esp_err_t 設定結果
     */
    esp_err_t setConfig(const Config& config);

    /**
     * @brief データ送信
     * @param data 送信データ
     * @param timeout タイムアウト時間
     * @return esp_err_t 送信結果
     */
    esp_err_t write(const std::vector<uint8_t>& data, TickType_t timeout = portMAX_DELAY);

    /**
     * @brief 文字列送信
     * @param str 送信文字列
     * @param timeout タイムアウト時間
     * @return esp_err_t 送信結果
     */
    esp_err_t writeString(const std::string& str, TickType_t timeout = portMAX_DELAY);

    /**
     * @brief データ受信
     * @param data 受信データ格納先
     * @param max_length 最大受信長
     * @param timeout タイムアウト時間
     * @return size_t 実際に受信したバイト数
     */
    size_t read(std::vector<uint8_t>& data, size_t max_length, TickType_t timeout = portMAX_DELAY);

    /**
     * @brief 行読み取り
     * @param line 読み取った行格納先
     * @param timeout タイムアウト時間
     * @return bool 行を読み取れた場合true
     */
    bool readLine(std::string& line, TickType_t timeout = portMAX_DELAY);

    /**
     * @brief 利用可能なデータサイズ取得
     * @return size_t 受信バッファ内のデータサイズ
     */
    size_t available();

    /**
     * @brief 受信バッファクリア
     * @return esp_err_t クリア結果
     */
    esp_err_t flush();

    /**
     * @brief 送信完了待機
     * @param timeout タイムアウト時間
     * @return esp_err_t 待機結果
     */
    esp_err_t waitTxDone(TickType_t timeout = portMAX_DELAY);

    /**
     * @brief ボーレート設定
     * @param baudrate ボーレート
     * @return esp_err_t 設定結果
     */
    esp_err_t setBaudrate(uint32_t baudrate);

    /**
     * @brief ボーレート取得
     * @param baudrate ボーレート格納先
     * @return esp_err_t 取得結果
     */
    esp_err_t getBaudrate(uint32_t& baudrate);

    /**
     * @brief ブレーク送信
     * @param duration ブレーク持続時間（ビット単位）
     * @return esp_err_t 送信結果
     */
    esp_err_t sendBreak(int duration);

    /**
     * @brief パターン検出設定
     * @param pattern 検出パターン
     * @param pattern_length パターン長
     * @param gap_timeout ギャップタイムアウト
     * @param pre_idle アイドル時間（前）
     * @param post_idle アイドル時間（後）
     * @return esp_err_t 設定結果
     */
    esp_err_t setPatternDetect(const char* pattern, size_t pattern_length, 
                              int gap_timeout, int pre_idle, int post_idle);

    /**
     * @brief パターン検出無効化
     * @return esp_err_t 無効化結果
     */
    esp_err_t disablePatternDetect();

    /**
     * @brief イベントコールバック設定
     * @param callback コールバック関数
     * @return esp_err_t 設定結果
     */
    esp_err_t setEventCallback(EventCallback callback);

    /**
     * @brief RS485モード設定
     * @param enable RS485モード有効化
     * @param tx_time 送信時間
     * @param rx_time 受信時間
     * @return esp_err_t 設定結果
     */
    esp_err_t setRS485Mode(bool enable, int tx_time = 0, int rx_time = 0);

    /**
     * @brief UARTポート番号取得
     * @return uart_port_t UARTポート番号
     */
    uart_port_t getPort() const { return config_.port; }

private:
    Config config_;                 // UART設定
    std::mutex mutex_;              // スレッドセーフ用ミューテックス
    bool driver_installed_;         // ドライバインストール状態
    QueueHandle_t event_queue_;     // イベントキュー
    EventCallback event_callback_;  // イベントコールバック
    TaskHandle_t event_task_;       // イベントタスクハンドル
    
    /**
     * @brief イベントタスク
     * @param arg 引数（UartHalインスタンス）
     */
    static void eventTask(void* arg);
};

} // namespace hal

#endif // UART_HAL_HPP