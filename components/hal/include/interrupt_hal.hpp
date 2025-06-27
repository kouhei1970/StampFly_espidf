/*
 * Interrupt HAL Class
 * 
 * 割り込み管理用のハードウェア抽象化レイヤー
 * ESP-IDF Interrupt APIをC++クラスでラップ
 * 
 * 作成者: Kouhei Ito
 * ライセンス: MIT License
 * 
 * Copyright (c) 2025 Kouhei Ito
 */

#ifndef INTERRUPT_HAL_HPP
#define INTERRUPT_HAL_HPP

#include "hal_base.hpp"
#include "esp_intr_alloc.h"
#include "esp_timer.h"
#include <functional>
#include <map>
#include <memory>
#include <mutex>

namespace hal {

/**
 * @brief Interrupt HALクラス
 * 
 * 割り込み管理の抽象化レイヤー
 * ESP-IDF Interrupt APIのC++ラッパー
 */
class InterruptHal : public HalBase {
public:
    /**
     * @brief 割り込み優先度列挙型
     */
    enum class Priority {
        LEVEL_1 = 1,    // 最低優先度
        LEVEL_2 = 2,
        LEVEL_3 = 3,
        LEVEL_4 = 4,
        LEVEL_5 = 5,    // 最高優先度（NMI以外）
        LEVEL_NMI = 7   // NMI（マスク不可能割り込み）
    };

    /**
     * @brief 割り込みフラグ列挙型
     */
    enum class Flags {
        NONE = 0,
        LEVEL = ESP_INTR_FLAG_LEVEL1,       // レベルトリガ
        EDGE = ESP_INTR_FLAG_EDGE,          // エッジトリガ
        SHARED = ESP_INTR_FLAG_SHARED,      // 共有割り込み
        IRAM = ESP_INTR_FLAG_IRAM,          // ハンドラはIRAMに配置
        HIGH_PRIORITY = ESP_INTR_FLAG_HIGH  // 高優先度
    };

    /**
     * @brief タイマー設定構造体
     */
    struct TimerConfig {
        uint64_t period_us;         // 周期（マイクロ秒）
        bool auto_reload;           // 自動リロード
        Priority priority;          // 割り込み優先度
        bool run_in_isr;           // ISR内で実行
    };

    /**
     * @brief 割り込みソース設定構造体
     */
    struct SourceConfig {
        int source;                 // 割り込みソース番号
        Priority priority;          // 優先度
        uint32_t flags;            // フラグ
    };

    /**
     * @brief 割り込みハンドラ関数型
     */
    using InterruptHandler = std::function<void(void)>;
    
    /**
     * @brief タイマーコールバック関数型
     */
    using TimerCallback = std::function<void(void)>;

    /**
     * @brief 割り込み統計情報構造体
     */
    struct Statistics {
        uint64_t total_count;       // 総割り込み回数
        uint64_t missed_count;      // 取りこぼし回数
        uint64_t max_latency_us;    // 最大遅延時間
        uint64_t avg_latency_us;    // 平均遅延時間
    };

public:
    /**
     * @brief コンストラクタ
     */
    InterruptHal();

    /**
     * @brief デストラクタ
     */
    virtual ~InterruptHal();

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
     * @brief 高精度タイマー作成
     * @param timer_id タイマーID
     * @param config タイマー設定
     * @param callback コールバック関数
     * @return esp_err_t 作成結果
     */
    esp_err_t createHighResTimer(uint32_t timer_id, const TimerConfig& config, TimerCallback callback);

    /**
     * @brief タイマー開始
     * @param timer_id タイマーID
     * @return esp_err_t 開始結果
     */
    esp_err_t startTimer(uint32_t timer_id);

    /**
     * @brief タイマー停止
     * @param timer_id タイマーID
     * @return esp_err_t 停止結果
     */
    esp_err_t stopTimer(uint32_t timer_id);

    /**
     * @brief タイマー削除
     * @param timer_id タイマーID
     * @return esp_err_t 削除結果
     */
    esp_err_t deleteTimer(uint32_t timer_id);

    /**
     * @brief タイマー周期変更
     * @param timer_id タイマーID
     * @param period_us 新しい周期（マイクロ秒）
     * @return esp_err_t 変更結果
     */
    esp_err_t setTimerPeriod(uint32_t timer_id, uint64_t period_us);

    /**
     * @brief 割り込みハンドラ登録
     * @param interrupt_id 割り込みID
     * @param config 割り込み設定
     * @param handler ハンドラ関数
     * @return esp_err_t 登録結果
     */
    esp_err_t registerInterrupt(uint32_t interrupt_id, const SourceConfig& config, InterruptHandler handler);

    /**
     * @brief 割り込みハンドラ解除
     * @param interrupt_id 割り込みID
     * @return esp_err_t 解除結果
     */
    esp_err_t unregisterInterrupt(uint32_t interrupt_id);

    /**
     * @brief 割り込み有効化
     * @param interrupt_id 割り込みID
     * @return esp_err_t 有効化結果
     */
    esp_err_t enableInterrupt(uint32_t interrupt_id);

    /**
     * @brief 割り込み無効化
     * @param interrupt_id 割り込みID
     * @return esp_err_t 無効化結果
     */
    esp_err_t disableInterrupt(uint32_t interrupt_id);

    /**
     * @brief 全割り込み無効化
     * @return uint32_t 以前の割り込み状態
     */
    uint32_t disableAllInterrupts();

    /**
     * @brief 割り込み状態復元
     * @param state 復元する割り込み状態
     */
    void restoreInterrupts(uint32_t state);

    /**
     * @brief 割り込み優先度設定
     * @param interrupt_id 割り込みID
     * @param priority 新しい優先度
     * @return esp_err_t 設定結果
     */
    esp_err_t setPriority(uint32_t interrupt_id, Priority priority);

    /**
     * @brief CPU親和性設定
     * @param interrupt_id 割り込みID
     * @param cpu_mask CPUマスク（ビット0: CPU0, ビット1: CPU1）
     * @return esp_err_t 設定結果
     */
    esp_err_t setCpuAffinity(uint32_t interrupt_id, uint32_t cpu_mask);

    /**
     * @brief 統計情報取得
     * @param interrupt_id 割り込みID
     * @param stats 統計情報格納先
     * @return esp_err_t 取得結果
     */
    esp_err_t getStatistics(uint32_t interrupt_id, Statistics& stats);

    /**
     * @brief 統計情報リセット
     * @param interrupt_id 割り込みID
     * @return esp_err_t リセット結果
     */
    esp_err_t resetStatistics(uint32_t interrupt_id);

    /**
     * @brief 現在のCPU取得
     * @return int 現在のCPU番号
     */
    static int getCurrentCpu();

    /**
     * @brief ISR内実行確認
     * @return bool ISR内で実行中の場合true
     */
    static bool isInIsr();

    /**
     * @brief クリティカルセクション開始
     * @return portMUX_TYPE* ミューテックスポインタ
     */
    portMUX_TYPE* enterCriticalSection();

    /**
     * @brief クリティカルセクション終了
     * @param mux ミューテックスポインタ
     */
    void exitCriticalSection(portMUX_TYPE* mux);

private:
    /**
     * @brief タイマー情報構造体
     */
    struct TimerInfo {
        esp_timer_handle_t handle;
        TimerConfig config;
        TimerCallback callback;
        Statistics stats;
    };

    /**
     * @brief 割り込み情報構造体
     */
    struct InterruptInfo {
        intr_handle_t handle;
        SourceConfig config;
        InterruptHandler handler;
        Statistics stats;
    };

    std::mutex mutex_;                              // スレッドセーフ用ミューテックス
    std::map<uint32_t, TimerInfo> timers_;        // タイマー管理
    std::map<uint32_t, InterruptInfo> interrupts_; // 割り込み管理
    portMUX_TYPE critical_mux_;                    // クリティカルセクション用
    
    /**
     * @brief ESPタイマーコールバック
     * @param arg 引数（タイマーID）
     */
    static void espTimerCallback(void* arg);
    
    /**
     * @brief 割り込みハンドララッパー
     * @param arg 引数（割り込みID）
     */
    static void IRAM_ATTR interruptHandlerWrapper(void* arg);
};

} // namespace hal

#endif // INTERRUPT_HAL_HPP