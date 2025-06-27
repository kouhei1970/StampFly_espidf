# StampFly ESP-IDF版 C++クラス設計書

## 概要

本プロジェクトはオブジェクト指向設計を採用し、C++クラスベースの構造で実装されます。各コンポーネントは独立したクラスとして設計され、カプセル化、継承、多態性の原則に従います。

## 設計原則

### 1. カプセル化
- 各クラスは明確に定義された責任範囲を持つ
- 内部実装の詳細は隠蔽し、公開インターフェースを最小限に保つ
- プライベートメンバ変数とパブリックメソッドで適切な境界を設定

### 2. 継承
- 共通機能を持つクラス群は基底クラスから継承
- センサークラス群は `SensorBase` から継承
- 制御クラス群は `ControllerBase` から継承

### 3. 多態性
- 仮想関数を使用して統一されたインターフェースを提供
- 異なる種類のセンサーや制御器を同じ方法で扱える

### 4. 単一責任の原則
- 各クラスは単一の明確な責任を持つ
- クラスの変更理由は一つに限定される

### 5. 依存性注入
- クラス間の結合度を最小化
- コンストラクタやセッターを通じて依存関係を注入

## 階層アーキテクチャ

```
Application Layer
    ↓
Control & Estimation Layer
    ↓
Sensor & Communication Layer
    ↓
Hardware Abstraction Layer (HAL)
    ↓
ESP-IDF APIs
```

## コア基底クラス

### SensorBase クラス
```cpp
class SensorBase {
public:
    virtual ~SensorBase() = default;
    virtual esp_err_t initialize() = 0;
    virtual esp_err_t read() = 0;
    virtual esp_err_t calibrate() = 0;
    virtual bool isDataReady() const = 0;
    virtual SensorStatus getStatus() const = 0;
    
protected:
    SensorStatus status_;
    uint32_t last_update_time_;
    bool is_initialized_;
};
```

### ControllerBase クラス
```cpp
class ControllerBase {
public:
    virtual ~ControllerBase() = default;
    virtual esp_err_t initialize() = 0;
    virtual void reset() = 0;
    virtual void setTarget(const ControlTarget& target) = 0;
    virtual ControlOutput compute(const ControlInput& input) = 0;
    
protected:
    bool is_enabled_;
    ControlParameters params_;
};
```

### HalBase クラス
```cpp
class HalBase {
public:
    virtual ~HalBase() = default;
    virtual esp_err_t initialize() = 0;
    virtual esp_err_t deinitialize() = 0;
    virtual bool isInitialized() const = 0;
    
protected:
    bool is_initialized_;
    HalConfig config_;
};
```

## センサークラス設計

### Imu クラス (BMI270)
```cpp
class Imu : public SensorBase {
public:
    Imu(std::shared_ptr<I2cHal> i2c_hal);
    ~Imu() override = default;
    
    esp_err_t initialize() override;
    esp_err_t read() override;
    esp_err_t calibrate() override;
    bool isDataReady() const override;
    
    // IMU固有のメソッド
    ImuData getData() const;
    esp_err_t setAccelRange(AccelRange range);
    esp_err_t setGyroRange(GyroRange range);
    esp_err_t setSampleRate(uint16_t rate);
    
private:
    std::shared_ptr<I2cHal> i2c_hal_;
    ImuData data_;
    ImuConfig config_;
    
    esp_err_t configureBMI270();
    esp_err_t readAccelData();
    esp_err_t readGyroData();
};
```

### Magnetometer クラス (BMM150)
```cpp
class Magnetometer : public SensorBase {
public:
    Magnetometer(std::shared_ptr<I2cHal> i2c_hal);
    ~Magnetometer() override = default;
    
    esp_err_t initialize() override;
    esp_err_t read() override;
    esp_err_t calibrate() override;
    bool isDataReady() const override;
    
    // 磁気センサー固有のメソッド
    MagData getData() const;
    esp_err_t startCalibration();
    bool isCalibrationComplete() const;
    CalibrationData getCalibrationData() const;
    
private:
    std::shared_ptr<I2cHal> i2c_hal_;
    MagData data_;
    CalibrationData calibration_;
    bool calibration_active_;
};
```

## HAL クラス設計

### I2cHal クラス
```cpp
class I2cHal : public HalBase {
public:
    I2cHal(i2c_port_t port, gpio_num_t sda_pin, gpio_num_t scl_pin);
    ~I2cHal() override;
    
    esp_err_t initialize() override;
    esp_err_t deinitialize() override;
    
    // I2C操作メソッド
    esp_err_t write(uint8_t device_addr, uint8_t reg_addr, 
                   const uint8_t* data, size_t len);
    esp_err_t read(uint8_t device_addr, uint8_t reg_addr, 
                  uint8_t* data, size_t len);
    esp_err_t writeRead(uint8_t device_addr, uint8_t reg_addr,
                       const uint8_t* write_data, size_t write_len,
                       uint8_t* read_data, size_t read_len);
    
private:
    i2c_port_t port_;
    gpio_num_t sda_pin_;
    gpio_num_t scl_pin_;
    i2c_config_t config_;
    
    esp_err_t configureDriver();
};
```

### GpioHal クラス
```cpp
class GpioHal : public HalBase {
public:
    GpioHal();
    ~GpioHal() override = default;
    
    esp_err_t initialize() override;
    esp_err_t deinitialize() override;
    
    // GPIO操作メソッド
    esp_err_t setDirection(gpio_num_t pin, gpio_mode_t mode);
    esp_err_t setLevel(gpio_num_t pin, uint32_t level);
    int getLevel(gpio_num_t pin);
    esp_err_t setPullMode(gpio_num_t pin, gpio_pull_mode_t pull);
    esp_err_t setIntrType(gpio_num_t pin, gpio_int_type_t intr_type);
    
private:
    std::map<gpio_num_t, GpioConfig> configured_pins_;
};
```

## 制御クラス設計

### PidController クラス
```cpp
class PidController : public ControllerBase {
public:
    PidController(const PidParams& params);
    ~PidController() override = default;
    
    esp_err_t initialize() override;
    void reset() override;
    void setTarget(const ControlTarget& target) override;
    ControlOutput compute(const ControlInput& input) override;
    
    // PID固有のメソッド
    void setGains(float kp, float ki, float kd);
    void setOutputLimits(float min, float max);
    void setIntegralLimits(float min, float max);
    PidStatus getStatus() const;
    
private:
    PidParams params_;
    float target_;
    float integral_;
    float previous_error_;
    float output_min_;
    float output_max_;
    uint32_t last_time_;
    
    float computeProportional(float error);
    float computeIntegral(float error, float dt);
    float computeDerivative(float error, float dt);
};
```

### FlightController クラス
```cpp
class FlightController {
public:
    FlightController(std::shared_ptr<SensorManager> sensor_mgr,
                    std::shared_ptr<ControlManager> control_mgr);
    ~FlightController() = default;
    
    esp_err_t initialize();
    void start();
    void stop();
    
    // メイン制御ループ
    void loop400Hz();
    
    // 状態管理
    void setState(FlightState state);
    FlightState getState() const;
    
private:
    std::shared_ptr<SensorManager> sensor_manager_;
    std::shared_ptr<ControlManager> control_manager_;
    std::shared_ptr<AttitudeEstimator> attitude_estimator_;
    std::shared_ptr<PositionEstimator> position_estimator_;
    
    FlightState current_state_;
    TaskHandle_t control_task_handle_;
    
    static void controlTaskWrapper(void* parameter);
    void processControlLoop();
    void updateEstimators();
    void computeControl();
    void updateActuators();
};
```

## マネージャークラス設計

### SensorManager クラス
```cpp
class SensorManager {
public:
    SensorManager();
    ~SensorManager() = default;
    
    esp_err_t initialize();
    void registerSensor(std::shared_ptr<SensorBase> sensor);
    void updateAll();
    
    // センサーアクセス
    std::shared_ptr<Imu> getImu() const { return imu_; }
    std::shared_ptr<Magnetometer> getMagnetometer() const { return magnetometer_; }
    std::shared_ptr<ToF> getToF() const { return tof_; }
    std::shared_ptr<Pressure> getPressure() const { return pressure_; }
    
    // 融合データ取得
    SensorFusionData getFusedData() const;
    
private:
    std::shared_ptr<Imu> imu_;
    std::shared_ptr<Magnetometer> magnetometer_;
    std::shared_ptr<ToF> tof_;
    std::shared_ptr<Pressure> pressure_;
    std::shared_ptr<PowerMonitor> power_monitor_;
    
    std::vector<std::shared_ptr<SensorBase>> sensors_;
    mutable std::mutex data_mutex_;
};
```

## 名前空間設計

```cpp
namespace stampfly {
namespace hal {
    // HAL クラス群
}

namespace sensors {
    // センサークラス群
}

namespace control {
    // 制御クラス群
}

namespace common {
    // 共通ユーティリティ
}

namespace communication {
    // 通信クラス群
}
}
```

## ファイル構成規則

### ヘッダーファイル (.hpp)
- クラス宣言
- インライン関数
- テンプレート実装

### ソースファイル (.cpp)
- クラス実装
- 非インライン関数

### ディレクトリ構造
```
components/
├── hal/
│   ├── include/hal/
│   │   ├── hal_base.hpp
│   │   ├── i2c_hal.hpp
│   │   └── gpio_hal.hpp
│   └── src/
│       ├── i2c_hal.cpp
│       └── gpio_hal.cpp
├── sensors/
│   ├── include/sensors/
│   │   ├── sensor_base.hpp
│   │   ├── imu.hpp
│   │   └── magnetometer.hpp
│   └── src/
│       ├── imu.cpp
│       └── magnetometer.cpp
└── common/
    ├── include/common/
    │   ├── types.hpp
    │   └── constants.hpp
    └── src/
        └── utilities.cpp
```

## エラーハンドリング

### エラー戦略
- ESP-IDF の `esp_err_t` を基本とする
- カスタム例外クラスは使用せず、戻り値でエラー処理
- ログレベルを適切に設定

### ログ戦略
```cpp
static const char* TAG = "ClassNamespace::ClassName";

esp_err_t MyClass::initialize() {
    ESP_LOGI(TAG, "初期化開始");
    
    esp_err_t ret = someOperation();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "操作失敗: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ESP_LOGI(TAG, "初期化完了");
    return ESP_OK;
}
```

## メモリ管理

### スマートポインタ使用
- `std::shared_ptr`: 複数所有が必要な場合
- `std::unique_ptr`: 単一所有の場合
- 生ポインタは避ける

### RAII (Resource Acquisition Is Initialization)
- コンストラクタでリソース取得
- デストラクタでリソース解放
- 例外安全性の確保

この設計書に基づいて、段階的にC++クラスベースの実装を進めていきます。