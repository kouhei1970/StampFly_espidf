# Flight Control（飛行制御）コンポーネント CMakeLists.txt
# 統合飛行制御システム、状態マシン、モーター制御

# コンポーネント情報
set(COMPONENT_NAME "flight_control")

# ソースファイル
set(COMPONENT_SRCS 
    "src/flight_control.c"
)

# インクルードディレクトリ（パブリック）
set(COMPONENT_ADD_INCLUDEDIRS 
    "include"
)

# コンポーネント依存関係
set(COMPONENT_REQUIRES
    freertos
    log
    esp_timer
    driver              # PWM制御
    control_manager     # 制御管理
    attitude_estimation # 姿勢推定
    position_estimation # 位置推定
    hal                # HAL層
    common             # 共通ユーティリティ
)

# コンポーネント登録
idf_component_register(
    SRCS ${COMPONENT_SRCS}
    INCLUDE_DIRS ${COMPONENT_ADD_INCLUDEDIRS}
    REQUIRES ${COMPONENT_REQUIRES}
)

# コンパイルオプション
target_compile_options(${COMPONENT_LIB} PRIVATE
    -Wall
    -Wextra
    -Werror=return-type
    -O2              # バランス最適化
    -ffast-math      # 数学演算最適化
)

# リンクライブラリ
target_link_libraries(${COMPONENT_LIB} PUBLIC
    m  # 数学ライブラリ
)

# プリプロセッサ定義
target_compile_definitions(${COMPONENT_LIB} PRIVATE
    FLIGHT_CONTROL_VERSION_MAJOR=1
    FLIGHT_CONTROL_VERSION_MINOR=0
    FLIGHT_CONTROL_VERSION_PATCH=0
    FLIGHT_CONTROL_FREQUENCY_HZ=400    # メイン制御ループ
    MOTOR_PWM_FREQUENCY_HZ=150000      # モーターPWM周波数
    MOTOR_PWM_RESOLUTION_BITS=8        # PWM分解能
    NUM_MOTORS=4                       # モーター数
    ENABLE_SAFETY_MONITOR=1            # 安全監視有効
    ENABLE_IMPACT_DETECTION=1          # 衝撃検出有効
    ENABLE_LOW_BATTERY_DETECTION=1     # 低バッテリー検出有効
    SAFETY_CHECK_FREQUENCY_HZ=100      # 安全チェック頻度
    EMERGENCY_STOP_TIME_MS=50          # 緊急停止時間
)