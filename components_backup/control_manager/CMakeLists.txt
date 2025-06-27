# Control Manager（制御管理）コンポーネント CMakeLists.txt
# PID/MPC制御の切り替え、制御性能監視、フェイルセーフ

# コンポーネント情報
set(COMPONENT_NAME "control_manager")

# ソースファイル
set(COMPONENT_SRCS 
    "src/control_manager.c"
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
    pid_control         # PID制御
    mpc_control         # MPC制御
    attitude_estimation # 姿勢推定
    position_estimation # 位置推定
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
)

# プリプロセッサ定義
target_compile_definitions(${COMPONENT_LIB} PRIVATE
    CONTROL_MANAGER_VERSION_MAJOR=1
    CONTROL_MANAGER_VERSION_MINOR=0
    CONTROL_MANAGER_VERSION_PATCH=0
    CONTROL_SWITCH_FREQUENCY_HZ=10     # 制御切り替え判定頻度
    PERFORMANCE_MONITOR_FREQUENCY_HZ=50 # 性能監視頻度
    ENABLE_AUTOMATIC_SWITCHING=1        # 自動切り替え有効
    ENABLE_PERFORMANCE_MONITORING=1     # 性能監視有効
    ENABLE_HEALTH_MONITORING=1          # 健全性監視有効
    FAILSAFE_TRIGGER_TIME_MS=100       # フェイルセーフ発動時間
    CONTROLLER_SWITCH_HYSTERESIS=1     # ヒステリシス有効
)