# StampFly ESP-IDF版

ESP32-S3ベースのクアッドコプタードローン制御システム

## プロジェクト概要

StampFly ESP-IDF版は、M5Stack StampS3（ESP32-S3）を使用した高性能ドローン制御システムです。Arduino版からESP-IDFネイティブに移植することで、より高度な制御性能とリアルタイム特性を実現します。

### 主要機能

- **高精度姿勢制御**: Madgwick AHRS + 拡張カルマンフィルター
- **位置推定システム**: 複数センサー融合による3D位置推定
- **デュアル制御システム**: PID制御 + MPC制御の切り替え対応
- **包括的CLI**: リアルタイム監視・チューニング・データストリーミング
- **多様な通信**: ESP-NOW、WiFi、Bluetooth対応
- **安全機能**: 多層フェイルセーフ・緊急停止システム

### ハードウェア仕様

- **メインMCU**: ESP32-S3 (240MHz デュアルコア)
- **センサー群**:
  - IMU: BMI270 (6軸 加速度・ジャイロ)
  - 磁気センサー: BMM150 (3軸)
  - 距離センサー: VL53L3CX ToF (下向き・前向き)
  - 気圧センサー: BMP280
  - 電力監視: INA3221
- **通信**: USB CDC、ESP-NOW、WiFi、Bluetooth
- **制御**: 4x PWMモーター制御、LED制御、ブザー

## ビルド方法

### 必要環境

- ESP-IDF v5.0以上
- CMake 3.16以上
- Python 3.7以上

### セットアップ

```bash
# ESP-IDF環境設定
source $IDF_PATH/export.sh

# プロジェクトビルド
cd StampFly_espidf
idf.py build

# フラッシュ書き込み
idf.py flash

# シリアルモニター開始
idf.py monitor
```

### 設定

```bash
# プロジェクト設定
idf.py menuconfig

# デバッグビルド
idf.py -DCMAKE_BUILD_TYPE=Debug build

# リリースビルド
idf.py -DCMAKE_BUILD_TYPE=Release build
```

## アーキテクチャ

### システム構成

```
Main Application (400Hz制御ループ)
├── Flight Control System (飛行制御)
├── Attitude Estimation (姿勢推定)
├── Position Estimation (位置推定)
├── Navigation System (ナビゲーション)
├── Control Manager (制御管理)
│   ├── PID Control (PID制御)
│   └── MPC Control (MPC制御)
├── Sensor Fusion (センサー融合)
├── Communication (通信)
├── CLI Interface (コマンドライン)
└── Safety Monitor (安全監視)
```

### タスク構成

- **制御タスク** (400Hz, Core1): リアルタイム飛行制御
- **センサータスク** (1000Hz, Core0): 高頻度センサー読み取り
- **メインタスク** (10Hz, Core1): システム管理・状態監視
- **CLIタスク** (100Hz, Core0): コマンド処理・通信

## 開発状況

### Phase 1: 基盤構築 ✅
- [x] プロジェクト構造設計
- [x] 基本ビルドシステム
- [x] タスク構成設計
- [ ] HAL層実装

### Phase 2: コア機能移植 🚧
- [ ] センサードライバー移植
- [ ] 姿勢推定システム
- [ ] 基本制御ループ
- [ ] PID制御システム

### Phase 3: 高度機能 📋
- [ ] 位置推定システム
- [ ] MPC制御システム
- [ ] ナビゲーション機能
- [ ] 通信システム

### Phase 4: 統合・最適化 📋
- [ ] 全機能統合テスト
- [ ] パフォーマンス最適化
- [ ] 安全機能検証
- [ ] ドキュメント整備

## ライセンス

MIT License

## 作成者

- **Kouhei Ito** - 開発者

## 関連リンク

- [Arduino版StampFly](../StampFly_sandbox/)
- [ESP-IDF公式ドキュメント](https://docs.espressif.com/projects/esp-idf/)
- [M5Stack StampS3](https://docs.m5stack.com/en/core/StampS3)# StampFly ESP-IDF Update
