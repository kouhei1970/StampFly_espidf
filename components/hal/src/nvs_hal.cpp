/*
 * NVS HAL Class Implementation
 * 
 * NVS（不揮発性ストレージ）用のハードウェア抽象化レイヤー実装
 * 
 * 作成者: Kouhei Ito
 * ライセンス: MIT License
 * 
 * Copyright (c) 2025 Kouhei Ito
 */

#include "nvs_hal.hpp"
#include "esp_log.h"
#include <cstring>

namespace hal {

NvsHal::NvsHal(const char* partition_label) 
    : HalBase("NVS_HAL")
    , nvs_initialized_(false) {
    if (partition_label) {
        partition_label_ = partition_label;
    }
    
    logDebug("NVS HALクラス作成 パーティション:%s", 
             partition_label_.empty() ? "デフォルト" : partition_label_.c_str());
}

NvsHal::~NvsHal() {
    // 全名前空間をクローズ
    closeAllNamespaces();
    
    logDebug("NVS HALクラス破棄");
}

esp_err_t NvsHal::initialize() {
    setState(State::INITIALIZING);
    
    // NVSフラッシュを初期化
    esp_err_t ret;
    if (partition_label_.empty()) {
        ret = nvs_flash_init();
    } else {
        ret = nvs_flash_init_partition(partition_label_.c_str());
    }
    
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // NVSパーティションを消去して再初期化
        logWarning("NVSパーティション消去が必要です");
        if (partition_label_.empty()) {
            ESP_ERROR_CHECK(nvs_flash_erase());
            ret = nvs_flash_init();
        } else {
            ESP_ERROR_CHECK(nvs_flash_erase_partition(partition_label_.c_str()));
            ret = nvs_flash_init_partition(partition_label_.c_str());
        }
    }
    
    if (ret != ESP_OK) {
        logError("NVS初期化失敗: %s", esp_err_to_name(ret));
        setState(State::ERROR);
        return ret;
    }
    
    nvs_initialized_ = true;
    setState(State::INITIALIZED);
    logInfo("NVS HAL初期化完了");
    return ESP_OK;
}

esp_err_t NvsHal::configure() {
    if (!isInitialized()) {
        logError("NVS HALが初期化されていません");
        return ESP_ERR_INVALID_STATE;
    }
    
    logInfo("NVS HAL設定完了");
    return ESP_OK;
}

esp_err_t NvsHal::start() {
    if (!isInitialized()) {
        return ESP_ERR_INVALID_STATE;
    }
    
    setState(State::RUNNING);
    logInfo("NVS HAL開始");
    return ESP_OK;
}

esp_err_t NvsHal::stop() {
    // 全名前空間の変更をコミット
    commitAll();
    
    setState(State::SUSPENDED);
    logInfo("NVS HAL停止");
    return ESP_OK;
}

esp_err_t NvsHal::reset() {
    // 全名前空間をクローズ
    closeAllNamespaces();
    
    setState(State::INITIALIZED);
    logInfo("NVS HALリセット完了");
    return ESP_OK;
}

esp_err_t NvsHal::openNamespace(const std::string& namespace_name, AccessMode mode) {
    if (!isRunning()) {
        logError("NVS HALが動作していません");
        return ESP_ERR_INVALID_STATE;
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    // 既に開いている場合はそのまま返す
    if (namespace_handles_.find(namespace_name) != namespace_handles_.end()) {
        logDebug("名前空間は既に開いています: %s", namespace_name.c_str());
        return ESP_OK;
    }
    
    nvs_handle_t handle;
    esp_err_t ret = nvs_open(namespace_name.c_str(), static_cast<nvs_open_mode_t>(mode), &handle);
    if (ret != ESP_OK) {
        logError("名前空間オープン失敗 %s: %s", namespace_name.c_str(), esp_err_to_name(ret));
        return ret;
    }
    
    namespace_handles_[namespace_name] = handle;
    logInfo("名前空間オープン: %s モード:%s", namespace_name.c_str(), 
            mode == AccessMode::READONLY ? "読み取り専用" : "読み書き");
    
    return ESP_OK;
}

esp_err_t NvsHal::closeNamespace(const std::string& namespace_name) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = namespace_handles_.find(namespace_name);
    if (it == namespace_handles_.end()) {
        logWarning("名前空間が開いていません: %s", namespace_name.c_str());
        return ESP_ERR_INVALID_STATE;
    }
    
    nvs_close(it->second);
    namespace_handles_.erase(it);
    logInfo("名前空間クローズ: %s", namespace_name.c_str());
    
    return ESP_OK;
}

esp_err_t NvsHal::closeAllNamespaces() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    for (const auto& pair : namespace_handles_) {
        nvs_close(pair.second);
    }
    namespace_handles_.clear();
    
    logInfo("全名前空間クローズ");
    return ESP_OK;
}

// 整数型の読み書き実装
esp_err_t NvsHal::writeInt8(const std::string& namespace_name, const std::string& key, int8_t value) {
    nvs_handle_t handle;
    esp_err_t ret = getNamespaceHandle(namespace_name, handle);
    if (ret != ESP_OK) return ret;
    
    ret = nvs_set_i8(handle, key.c_str(), value);
    if (ret != ESP_OK) {
        logError("int8書き込み失敗 %s/%s: %s", namespace_name.c_str(), key.c_str(), esp_err_to_name(ret));
    }
    return ret;
}

esp_err_t NvsHal::readInt8(const std::string& namespace_name, const std::string& key, int8_t& value) {
    nvs_handle_t handle;
    esp_err_t ret = getNamespaceHandle(namespace_name, handle);
    if (ret != ESP_OK) return ret;
    
    ret = nvs_get_i8(handle, key.c_str(), &value);
    if (ret != ESP_OK && ret != ESP_ERR_NVS_NOT_FOUND) {
        logError("int8読み取り失敗 %s/%s: %s", namespace_name.c_str(), key.c_str(), esp_err_to_name(ret));
    }
    return ret;
}

esp_err_t NvsHal::writeUInt8(const std::string& namespace_name, const std::string& key, uint8_t value) {
    nvs_handle_t handle;
    esp_err_t ret = getNamespaceHandle(namespace_name, handle);
    if (ret != ESP_OK) return ret;
    
    ret = nvs_set_u8(handle, key.c_str(), value);
    if (ret != ESP_OK) {
        logError("uint8書き込み失敗 %s/%s: %s", namespace_name.c_str(), key.c_str(), esp_err_to_name(ret));
    }
    return ret;
}

esp_err_t NvsHal::readUInt8(const std::string& namespace_name, const std::string& key, uint8_t& value) {
    nvs_handle_t handle;
    esp_err_t ret = getNamespaceHandle(namespace_name, handle);
    if (ret != ESP_OK) return ret;
    
    ret = nvs_get_u8(handle, key.c_str(), &value);
    if (ret != ESP_OK && ret != ESP_ERR_NVS_NOT_FOUND) {
        logError("uint8読み取り失敗 %s/%s: %s", namespace_name.c_str(), key.c_str(), esp_err_to_name(ret));
    }
    return ret;
}

esp_err_t NvsHal::writeInt16(const std::string& namespace_name, const std::string& key, int16_t value) {
    nvs_handle_t handle;
    esp_err_t ret = getNamespaceHandle(namespace_name, handle);
    if (ret != ESP_OK) return ret;
    
    ret = nvs_set_i16(handle, key.c_str(), value);
    if (ret != ESP_OK) {
        logError("int16書き込み失敗 %s/%s: %s", namespace_name.c_str(), key.c_str(), esp_err_to_name(ret));
    }
    return ret;
}

esp_err_t NvsHal::readInt16(const std::string& namespace_name, const std::string& key, int16_t& value) {
    nvs_handle_t handle;
    esp_err_t ret = getNamespaceHandle(namespace_name, handle);
    if (ret != ESP_OK) return ret;
    
    ret = nvs_get_i16(handle, key.c_str(), &value);
    if (ret != ESP_OK && ret != ESP_ERR_NVS_NOT_FOUND) {
        logError("int16読み取り失敗 %s/%s: %s", namespace_name.c_str(), key.c_str(), esp_err_to_name(ret));
    }
    return ret;
}

esp_err_t NvsHal::writeUInt16(const std::string& namespace_name, const std::string& key, uint16_t value) {
    nvs_handle_t handle;
    esp_err_t ret = getNamespaceHandle(namespace_name, handle);
    if (ret != ESP_OK) return ret;
    
    ret = nvs_set_u16(handle, key.c_str(), value);
    if (ret != ESP_OK) {
        logError("uint16書き込み失敗 %s/%s: %s", namespace_name.c_str(), key.c_str(), esp_err_to_name(ret));
    }
    return ret;
}

esp_err_t NvsHal::readUInt16(const std::string& namespace_name, const std::string& key, uint16_t& value) {
    nvs_handle_t handle;
    esp_err_t ret = getNamespaceHandle(namespace_name, handle);
    if (ret != ESP_OK) return ret;
    
    ret = nvs_get_u16(handle, key.c_str(), &value);
    if (ret != ESP_OK && ret != ESP_ERR_NVS_NOT_FOUND) {
        logError("uint16読み取り失敗 %s/%s: %s", namespace_name.c_str(), key.c_str(), esp_err_to_name(ret));
    }
    return ret;
}

esp_err_t NvsHal::writeInt32(const std::string& namespace_name, const std::string& key, int32_t value) {
    nvs_handle_t handle;
    esp_err_t ret = getNamespaceHandle(namespace_name, handle);
    if (ret != ESP_OK) return ret;
    
    ret = nvs_set_i32(handle, key.c_str(), value);
    if (ret != ESP_OK) {
        logError("int32書き込み失敗 %s/%s: %s", namespace_name.c_str(), key.c_str(), esp_err_to_name(ret));
    }
    return ret;
}

esp_err_t NvsHal::readInt32(const std::string& namespace_name, const std::string& key, int32_t& value) {
    nvs_handle_t handle;
    esp_err_t ret = getNamespaceHandle(namespace_name, handle);
    if (ret != ESP_OK) return ret;
    
    ret = nvs_get_i32(handle, key.c_str(), &value);
    if (ret != ESP_OK && ret != ESP_ERR_NVS_NOT_FOUND) {
        logError("int32読み取り失敗 %s/%s: %s", namespace_name.c_str(), key.c_str(), esp_err_to_name(ret));
    }
    return ret;
}

esp_err_t NvsHal::writeUInt32(const std::string& namespace_name, const std::string& key, uint32_t value) {
    nvs_handle_t handle;
    esp_err_t ret = getNamespaceHandle(namespace_name, handle);
    if (ret != ESP_OK) return ret;
    
    ret = nvs_set_u32(handle, key.c_str(), value);
    if (ret != ESP_OK) {
        logError("uint32書き込み失敗 %s/%s: %s", namespace_name.c_str(), key.c_str(), esp_err_to_name(ret));
    }
    return ret;
}

esp_err_t NvsHal::readUInt32(const std::string& namespace_name, const std::string& key, uint32_t& value) {
    nvs_handle_t handle;
    esp_err_t ret = getNamespaceHandle(namespace_name, handle);
    if (ret != ESP_OK) return ret;
    
    ret = nvs_get_u32(handle, key.c_str(), &value);
    if (ret != ESP_OK && ret != ESP_ERR_NVS_NOT_FOUND) {
        logError("uint32読み取り失敗 %s/%s: %s", namespace_name.c_str(), key.c_str(), esp_err_to_name(ret));
    }
    return ret;
}

esp_err_t NvsHal::writeInt64(const std::string& namespace_name, const std::string& key, int64_t value) {
    nvs_handle_t handle;
    esp_err_t ret = getNamespaceHandle(namespace_name, handle);
    if (ret != ESP_OK) return ret;
    
    ret = nvs_set_i64(handle, key.c_str(), value);
    if (ret != ESP_OK) {
        logError("int64書き込み失敗 %s/%s: %s", namespace_name.c_str(), key.c_str(), esp_err_to_name(ret));
    }
    return ret;
}

esp_err_t NvsHal::readInt64(const std::string& namespace_name, const std::string& key, int64_t& value) {
    nvs_handle_t handle;
    esp_err_t ret = getNamespaceHandle(namespace_name, handle);
    if (ret != ESP_OK) return ret;
    
    ret = nvs_get_i64(handle, key.c_str(), &value);
    if (ret != ESP_OK && ret != ESP_ERR_NVS_NOT_FOUND) {
        logError("int64読み取り失敗 %s/%s: %s", namespace_name.c_str(), key.c_str(), esp_err_to_name(ret));
    }
    return ret;
}

esp_err_t NvsHal::writeUInt64(const std::string& namespace_name, const std::string& key, uint64_t value) {
    nvs_handle_t handle;
    esp_err_t ret = getNamespaceHandle(namespace_name, handle);
    if (ret != ESP_OK) return ret;
    
    ret = nvs_set_u64(handle, key.c_str(), value);
    if (ret != ESP_OK) {
        logError("uint64書き込み失敗 %s/%s: %s", namespace_name.c_str(), key.c_str(), esp_err_to_name(ret));
    }
    return ret;
}

esp_err_t NvsHal::readUInt64(const std::string& namespace_name, const std::string& key, uint64_t& value) {
    nvs_handle_t handle;
    esp_err_t ret = getNamespaceHandle(namespace_name, handle);
    if (ret != ESP_OK) return ret;
    
    ret = nvs_get_u64(handle, key.c_str(), &value);
    if (ret != ESP_OK && ret != ESP_ERR_NVS_NOT_FOUND) {
        logError("uint64読み取り失敗 %s/%s: %s", namespace_name.c_str(), key.c_str(), esp_err_to_name(ret));
    }
    return ret;
}

// 浮動小数点型の読み書き実装
esp_err_t NvsHal::writeFloat(const std::string& namespace_name, const std::string& key, float value) {
    // floatをuint32_tとして保存
    uint32_t uint_value;
    memcpy(&uint_value, &value, sizeof(float));
    return writeUInt32(namespace_name, key, uint_value);
}

esp_err_t NvsHal::readFloat(const std::string& namespace_name, const std::string& key, float& value) {
    // uint32_tとして読み取り、floatに変換
    uint32_t uint_value;
    esp_err_t ret = readUInt32(namespace_name, key, uint_value);
    if (ret == ESP_OK) {
        memcpy(&value, &uint_value, sizeof(float));
    }
    return ret;
}

esp_err_t NvsHal::writeDouble(const std::string& namespace_name, const std::string& key, double value) {
    // doubleをuint64_tとして保存
    uint64_t uint_value;
    memcpy(&uint_value, &value, sizeof(double));
    return writeUInt64(namespace_name, key, uint_value);
}

esp_err_t NvsHal::readDouble(const std::string& namespace_name, const std::string& key, double& value) {
    // uint64_tとして読み取り、doubleに変換
    uint64_t uint_value;
    esp_err_t ret = readUInt64(namespace_name, key, uint_value);
    if (ret == ESP_OK) {
        memcpy(&value, &uint_value, sizeof(double));
    }
    return ret;
}

// 文字列の読み書き実装
esp_err_t NvsHal::writeString(const std::string& namespace_name, const std::string& key, const std::string& value) {
    nvs_handle_t handle;
    esp_err_t ret = getNamespaceHandle(namespace_name, handle);
    if (ret != ESP_OK) return ret;
    
    ret = nvs_set_str(handle, key.c_str(), value.c_str());
    if (ret != ESP_OK) {
        logError("文字列書き込み失敗 %s/%s: %s", namespace_name.c_str(), key.c_str(), esp_err_to_name(ret));
    }
    return ret;
}

esp_err_t NvsHal::readString(const std::string& namespace_name, const std::string& key, std::string& value) {
    nvs_handle_t handle;
    esp_err_t ret = getNamespaceHandle(namespace_name, handle);
    if (ret != ESP_OK) return ret;
    
    // サイズを取得
    size_t length = 0;
    ret = nvs_get_str(handle, key.c_str(), nullptr, &length);
    if (ret != ESP_OK) {
        if (ret != ESP_ERR_NVS_NOT_FOUND) {
            logError("文字列サイズ取得失敗 %s/%s: %s", namespace_name.c_str(), key.c_str(), esp_err_to_name(ret));
        }
        return ret;
    }
    
    // バッファを確保して読み取り
    std::vector<char> buffer(length);
    ret = nvs_get_str(handle, key.c_str(), buffer.data(), &length);
    if (ret != ESP_OK) {
        logError("文字列読み取り失敗 %s/%s: %s", namespace_name.c_str(), key.c_str(), esp_err_to_name(ret));
        return ret;
    }
    
    value = std::string(buffer.data());
    return ESP_OK;
}

// バイナリデータの読み書き実装
esp_err_t NvsHal::writeBlob(const std::string& namespace_name, const std::string& key, const std::vector<uint8_t>& data) {
    nvs_handle_t handle;
    esp_err_t ret = getNamespaceHandle(namespace_name, handle);
    if (ret != ESP_OK) return ret;
    
    ret = nvs_set_blob(handle, key.c_str(), data.data(), data.size());
    if (ret != ESP_OK) {
        logError("BLOB書き込み失敗 %s/%s: %s", namespace_name.c_str(), key.c_str(), esp_err_to_name(ret));
    }
    return ret;
}

esp_err_t NvsHal::readBlob(const std::string& namespace_name, const std::string& key, std::vector<uint8_t>& data) {
    nvs_handle_t handle;
    esp_err_t ret = getNamespaceHandle(namespace_name, handle);
    if (ret != ESP_OK) return ret;
    
    // サイズを取得
    size_t length = 0;
    ret = nvs_get_blob(handle, key.c_str(), nullptr, &length);
    if (ret != ESP_OK) {
        if (ret != ESP_ERR_NVS_NOT_FOUND) {
            logError("BLOBサイズ取得失敗 %s/%s: %s", namespace_name.c_str(), key.c_str(), esp_err_to_name(ret));
        }
        return ret;
    }
    
    // バッファを確保して読み取り
    data.resize(length);
    ret = nvs_get_blob(handle, key.c_str(), data.data(), &length);
    if (ret != ESP_OK) {
        logError("BLOB読み取り失敗 %s/%s: %s", namespace_name.c_str(), key.c_str(), esp_err_to_name(ret));
        data.clear();
        return ret;
    }
    
    return ESP_OK;
}

esp_err_t NvsHal::eraseKey(const std::string& namespace_name, const std::string& key) {
    nvs_handle_t handle;
    esp_err_t ret = getNamespaceHandle(namespace_name, handle);
    if (ret != ESP_OK) return ret;
    
    ret = nvs_erase_key(handle, key.c_str());
    if (ret != ESP_OK && ret != ESP_ERR_NVS_NOT_FOUND) {
        logError("キー削除失敗 %s/%s: %s", namespace_name.c_str(), key.c_str(), esp_err_to_name(ret));
    }
    return ret;
}

esp_err_t NvsHal::eraseNamespace(const std::string& namespace_name) {
    nvs_handle_t handle;
    esp_err_t ret = getNamespaceHandle(namespace_name, handle);
    if (ret != ESP_OK) return ret;
    
    ret = nvs_erase_all(handle);
    if (ret != ESP_OK) {
        logError("名前空間削除失敗 %s: %s", namespace_name.c_str(), esp_err_to_name(ret));
    }
    return ret;
}

esp_err_t NvsHal::eraseAll() {
    if (partition_label_.empty()) {
        return nvs_flash_erase();
    } else {
        return nvs_flash_erase_partition(partition_label_.c_str());
    }
}

esp_err_t NvsHal::commit(const std::string& namespace_name) {
    nvs_handle_t handle;
    esp_err_t ret = getNamespaceHandle(namespace_name, handle);
    if (ret != ESP_OK) return ret;
    
    ret = nvs_commit(handle);
    if (ret != ESP_OK) {
        logError("コミット失敗 %s: %s", namespace_name.c_str(), esp_err_to_name(ret));
    } else {
        logDebug("コミット成功 %s", namespace_name.c_str());
    }
    return ret;
}

esp_err_t NvsHal::commitAll() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    esp_err_t ret = ESP_OK;
    for (const auto& pair : namespace_handles_) {
        esp_err_t commit_ret = nvs_commit(pair.second);
        if (commit_ret != ESP_OK) {
            logError("コミット失敗 %s: %s", pair.first.c_str(), esp_err_to_name(commit_ret));
            ret = commit_ret;
        }
    }
    
    return ret;
}

bool NvsHal::hasKey(const std::string& namespace_name, const std::string& key) {
    nvs_handle_t handle;
    esp_err_t ret = getNamespaceHandle(namespace_name, handle);
    if (ret != ESP_OK) return false;
    
    // サイズ取得でキーの存在を確認
    size_t length = 0;
    ret = nvs_get_blob(handle, key.c_str(), nullptr, &length);
    
    return (ret == ESP_OK || ret == ESP_ERR_NVS_INVALID_LENGTH);
}

esp_err_t NvsHal::getDataSize(const std::string& namespace_name, const std::string& key, size_t& size) {
    nvs_handle_t handle;
    esp_err_t ret = getNamespaceHandle(namespace_name, handle);
    if (ret != ESP_OK) return ret;
    
    ret = nvs_get_blob(handle, key.c_str(), nullptr, &size);
    if (ret != ESP_OK && ret != ESP_ERR_NVS_INVALID_LENGTH) {
        logError("データサイズ取得失敗 %s/%s: %s", namespace_name.c_str(), key.c_str(), esp_err_to_name(ret));
    }
    
    return (ret == ESP_ERR_NVS_INVALID_LENGTH) ? ESP_OK : ret;
}

esp_err_t NvsHal::getStatistics(Statistics& stats) {
    nvs_stats_t nvs_stats = {};
    
    esp_err_t ret;
    if (partition_label_.empty()) {
        ret = nvs_get_stats(nullptr, &nvs_stats);
    } else {
        ret = nvs_get_stats(partition_label_.c_str(), &nvs_stats);
    }
    
    if (ret != ESP_OK) {
        logError("統計情報取得失敗: %s", esp_err_to_name(ret));
        return ret;
    }
    
    stats.used_entries = nvs_stats.used_entries;
    stats.free_entries = nvs_stats.free_entries;
    stats.total_entries = nvs_stats.total_entries;
    stats.namespace_count = nvs_stats.namespace_count;
    
    logInfo("NVS統計: 使用:%zu 空き:%zu 総数:%zu 名前空間:%zu",
            stats.used_entries, stats.free_entries, stats.total_entries, stats.namespace_count);
    
    return ESP_OK;
}

esp_err_t NvsHal::getNamespaceHandle(const std::string& namespace_name, nvs_handle_t& handle) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = namespace_handles_.find(namespace_name);
    if (it == namespace_handles_.end()) {
        // 自動的に名前空間を開く
        esp_err_t ret = nvs_open(namespace_name.c_str(), NVS_READWRITE, &handle);
        if (ret != ESP_OK) {
            logError("名前空間自動オープン失敗 %s: %s", namespace_name.c_str(), esp_err_to_name(ret));
            return ret;
        }
        namespace_handles_[namespace_name] = handle;
        logDebug("名前空間自動オープン: %s", namespace_name.c_str());
    } else {
        handle = it->second;
    }
    
    return ESP_OK;
}

} // namespace hal