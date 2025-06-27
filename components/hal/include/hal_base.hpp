/*
 * HAL Base Class - Hardware Abstraction Layer Base
 * 
 * HALクラス群の基底クラス
 * 共通のインターフェースと機能を提供
 * 
 * 作成者: Kouhei Ito
 * ライセンス: MIT License
 * 
 * Copyright (c) 2025 Kouhei Ito
 */

#ifndef HAL_BASE_HPP
#define HAL_BASE_HPP

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "esp_log.h"

namespace hal {

/**
 * @brief HAL基底クラス
 * 
 * すべてのHALクラスの共通基底クラス
 * 初期化、設定、エラーハンドリング等の共通機能を提供
 */
class HalBase {
public:
    /**
     * @brief HAL状態列挙型
     */
    enum class State {
        UNINITIALIZED,  // 未初期化
        INITIALIZING,   // 初期化中
        INITIALIZED,    // 初期化完了
        RUNNING,        // 動作中
        ERROR,          // エラー状態
        SUSPENDED       // 中断状態
    };

    /**
     * @brief HAL優先度列挙型
     */
    enum class Priority {
        LOW = 0,        // 低優先度
        NORMAL = 1,     // 通常優先度
        HIGH = 2,       // 高優先度
        CRITICAL = 3    // クリティカル優先度
    };

protected:
    /**
     * @brief コンストラクタ
     * @param component_name コンポーネント名（ログ用）
     */
    explicit HalBase(const char* component_name);

public:
    /**
     * @brief デストラクタ
     */
    virtual ~HalBase();

    /**
     * @brief 初期化（純粋仮想関数）
     * @return esp_err_t 初期化結果
     */
    virtual esp_err_t initialize() = 0;

    /**
     * @brief 設定変更（純粋仮想関数）
     * @return esp_err_t 設定結果
     */
    virtual esp_err_t configure() = 0;

    /**
     * @brief 開始（純粋仮想関数）
     * @return esp_err_t 開始結果
     */
    virtual esp_err_t start() = 0;

    /**
     * @brief 停止（純粋仮想関数）
     * @return esp_err_t 停止結果
     */
    virtual esp_err_t stop() = 0;

    /**
     * @brief リセット（純粋仮想関数）
     * @return esp_err_t リセット結果
     */
    virtual esp_err_t reset() = 0;

    /**
     * @brief 状態取得
     * @return State 現在の状態
     */
    State getState() const { return state_; }

    /**
     * @brief エラー状態確認
     * @return bool エラー状態の場合true
     */
    bool hasError() const { return state_ == State::ERROR; }

    /**
     * @brief 初期化済み確認
     * @return bool 初期化済みの場合true
     */
    bool isInitialized() const { 
        return state_ == State::INITIALIZED || state_ == State::RUNNING; 
    }

    /**
     * @brief 動作中確認
     * @return bool 動作中の場合true
     */
    bool isRunning() const { return state_ == State::RUNNING; }

    /**
     * @brief 優先度設定
     * @param priority 優先度
     */
    void setPriority(Priority priority) { priority_ = priority; }

    /**
     * @brief 優先度取得
     * @return Priority 現在の優先度
     */
    Priority getPriority() const { return priority_; }

    /**
     * @brief コンポーネント名取得
     * @return const char* コンポーネント名
     */
    const char* getComponentName() const { return component_name_; }

protected:
    /**
     * @brief 状態変更
     * @param new_state 新しい状態
     */
    void setState(State new_state);

    /**
     * @brief エラーログ出力
     * @param format フォーマット文字列
     * @param ... 可変引数
     */
    void logError(const char* format, ...);

    /**
     * @brief 警告ログ出力
     * @param format フォーマット文字列
     * @param ... 可変引数
     */
    void logWarning(const char* format, ...);

    /**
     * @brief 情報ログ出力
     * @param format フォーマット文字列
     * @param ... 可変引数
     */
    void logInfo(const char* format, ...);

    /**
     * @brief デバッグログ出力
     * @param format フォーマット文字列
     * @param ... 可変引数
     */
    void logDebug(const char* format, ...);

private:
    const char* component_name_;    // コンポーネント名
    State state_;                   // 現在の状態
    Priority priority_;             // 優先度
    
    // コピー禁止
    HalBase(const HalBase&) = delete;
    HalBase& operator=(const HalBase&) = delete;
};

} // namespace hal

#endif // HAL_BASE_HPP