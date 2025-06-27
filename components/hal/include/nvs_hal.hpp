/*
 * NVS HAL Class
 * 
 * NVS（不揮発性ストレージ）用のハードウェア抽象化レイヤー
 * ESP-IDF NVS APIをC++クラスでラップ
 * 
 * 作成者: Kouhei Ito
 * ライセンス: MIT License
 * 
 * Copyright (c) 2025 Kouhei Ito
 */

#ifndef NVS_HAL_HPP
#define NVS_HAL_HPP

#include "hal_base.hpp"
#include "nvs_flash.h"
#include "nvs.h"
#include <string>
#include <vector>
#include <memory>
#include <mutex>

namespace hal {

/**
 * @brief NVS HALクラス
 * 
 * NVS操作の抽象化レイヤー
 * ESP-IDF NVS APIのC++ラッパー
 */
class NvsHal : public HalBase {
public:
    /**
     * @brief NVSアクセスモード列挙型
     */
    enum class AccessMode {
        READONLY = NVS_READONLY,
        READWRITE = NVS_READWRITE
    };

    /**
     * @brief NVS名前空間設定構造体
     */
    struct NamespaceConfig {
        std::string name;           // 名前空間名
        AccessMode mode;            // アクセスモード
        std::string partition;      // パーティション名（オプション）
    };

    /**
     * @brief NVS統計情報構造体
     */
    struct Statistics {
        size_t used_entries;        // 使用済みエントリ数
        size_t free_entries;        // 空きエントリ数
        size_t total_entries;       // 総エントリ数
        size_t namespace_count;     // 名前空間数
    };

public:
    /**
     * @brief コンストラクタ
     * @param partition_label パーティションラベル（nullptrでデフォルト）
     */
    explicit NvsHal(const char* partition_label = nullptr);

    /**
     * @brief デストラクタ
     */
    virtual ~NvsHal();

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
     * @brief 名前空間オープン
     * @param namespace_name 名前空間名
     * @param mode アクセスモード
     * @return esp_err_t オープン結果
     */
    esp_err_t openNamespace(const std::string& namespace_name, AccessMode mode = AccessMode::READWRITE);

    /**
     * @brief 名前空間クローズ
     * @param namespace_name 名前空間名
     * @return esp_err_t クローズ結果
     */
    esp_err_t closeNamespace(const std::string& namespace_name);

    /**
     * @brief 全名前空間クローズ
     * @return esp_err_t クローズ結果
     */
    esp_err_t closeAllNamespaces();

    // 整数型の読み書き
    esp_err_t writeInt8(const std::string& namespace_name, const std::string& key, int8_t value);
    esp_err_t readInt8(const std::string& namespace_name, const std::string& key, int8_t& value);
    
    esp_err_t writeUInt8(const std::string& namespace_name, const std::string& key, uint8_t value);
    esp_err_t readUInt8(const std::string& namespace_name, const std::string& key, uint8_t& value);
    
    esp_err_t writeInt16(const std::string& namespace_name, const std::string& key, int16_t value);
    esp_err_t readInt16(const std::string& namespace_name, const std::string& key, int16_t& value);
    
    esp_err_t writeUInt16(const std::string& namespace_name, const std::string& key, uint16_t value);
    esp_err_t readUInt16(const std::string& namespace_name, const std::string& key, uint16_t& value);
    
    esp_err_t writeInt32(const std::string& namespace_name, const std::string& key, int32_t value);
    esp_err_t readInt32(const std::string& namespace_name, const std::string& key, int32_t& value);
    
    esp_err_t writeUInt32(const std::string& namespace_name, const std::string& key, uint32_t value);
    esp_err_t readUInt32(const std::string& namespace_name, const std::string& key, uint32_t& value);
    
    esp_err_t writeInt64(const std::string& namespace_name, const std::string& key, int64_t value);
    esp_err_t readInt64(const std::string& namespace_name, const std::string& key, int64_t& value);
    
    esp_err_t writeUInt64(const std::string& namespace_name, const std::string& key, uint64_t value);
    esp_err_t readUInt64(const std::string& namespace_name, const std::string& key, uint64_t& value);

    // 浮動小数点型の読み書き
    esp_err_t writeFloat(const std::string& namespace_name, const std::string& key, float value);
    esp_err_t readFloat(const std::string& namespace_name, const std::string& key, float& value);
    
    esp_err_t writeDouble(const std::string& namespace_name, const std::string& key, double value);
    esp_err_t readDouble(const std::string& namespace_name, const std::string& key, double& value);

    // 文字列の読み書き
    esp_err_t writeString(const std::string& namespace_name, const std::string& key, const std::string& value);
    esp_err_t readString(const std::string& namespace_name, const std::string& key, std::string& value);

    // バイナリデータの読み書き
    esp_err_t writeBlob(const std::string& namespace_name, const std::string& key, const std::vector<uint8_t>& data);
    esp_err_t readBlob(const std::string& namespace_name, const std::string& key, std::vector<uint8_t>& data);

    /**
     * @brief キー削除
     * @param namespace_name 名前空間名
     * @param key キー名
     * @return esp_err_t 削除結果
     */
    esp_err_t eraseKey(const std::string& namespace_name, const std::string& key);

    /**
     * @brief 名前空間内全削除
     * @param namespace_name 名前空間名
     * @return esp_err_t 削除結果
     */
    esp_err_t eraseNamespace(const std::string& namespace_name);

    /**
     * @brief 全データ削除
     * @return esp_err_t 削除結果
     */
    esp_err_t eraseAll();

    /**
     * @brief 変更をコミット
     * @param namespace_name 名前空間名
     * @return esp_err_t コミット結果
     */
    esp_err_t commit(const std::string& namespace_name);

    /**
     * @brief 全名前空間の変更をコミット
     * @return esp_err_t コミット結果
     */
    esp_err_t commitAll();

    /**
     * @brief キー存在確認
     * @param namespace_name 名前空間名
     * @param key キー名
     * @return bool キーが存在する場合true
     */
    bool hasKey(const std::string& namespace_name, const std::string& key);

    /**
     * @brief データサイズ取得
     * @param namespace_name 名前空間名
     * @param key キー名
     * @param size サイズ格納先
     * @return esp_err_t 取得結果
     */
    esp_err_t getDataSize(const std::string& namespace_name, const std::string& key, size_t& size);

    /**
     * @brief 統計情報取得
     * @param stats 統計情報格納先
     * @return esp_err_t 取得結果
     */
    esp_err_t getStatistics(Statistics& stats);

    /**
     * @brief 構造体の保存（テンプレート）
     * @tparam T 構造体型
     * @param namespace_name 名前空間名
     * @param key キー名
     * @param data 保存する構造体
     * @return esp_err_t 保存結果
     */
    template<typename T>
    esp_err_t writeStruct(const std::string& namespace_name, const std::string& key, const T& data) {
        std::vector<uint8_t> blob(sizeof(T));
        memcpy(blob.data(), &data, sizeof(T));
        return writeBlob(namespace_name, key, blob);
    }

    /**
     * @brief 構造体の読み取り（テンプレート）
     * @tparam T 構造体型
     * @param namespace_name 名前空間名
     * @param key キー名
     * @param data 読み取った構造体格納先
     * @return esp_err_t 読み取り結果
     */
    template<typename T>
    esp_err_t readStruct(const std::string& namespace_name, const std::string& key, T& data) {
        std::vector<uint8_t> blob;
        esp_err_t ret = readBlob(namespace_name, key, blob);
        if (ret == ESP_OK && blob.size() == sizeof(T)) {
            memcpy(&data, blob.data(), sizeof(T));
            return ESP_OK;
        }
        return ret == ESP_OK ? ESP_ERR_NVS_INVALID_LENGTH : ret;
    }

private:
    std::string partition_label_;                           // パーティションラベル
    std::mutex mutex_;                                      // スレッドセーフ用ミューテックス
    std::map<std::string, nvs_handle_t> namespace_handles_; // 名前空間ハンドル管理
    bool nvs_initialized_;                                  // NVS初期化状態
    
    /**
     * @brief 名前空間ハンドル取得
     * @param namespace_name 名前空間名
     * @param handle ハンドル格納先
     * @return esp_err_t 取得結果
     */
    esp_err_t getNamespaceHandle(const std::string& namespace_name, nvs_handle_t& handle);
};

} // namespace hal

#endif // NVS_HAL_HPP