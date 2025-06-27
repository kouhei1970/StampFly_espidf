# System（システム管理）コンポーネント CMakeLists.txt
# タスク管理、メモリ監視、パフォーマンス監視、診断

# コンポーネント情報
set(COMPONENT_NAME "system")

# ソースファイル
set(COMPONENT_SRCS 
    "src/diagnostic.c"
    "src/fault_handler.c"
    "src/memory_monitor.c"
    "src/performance_monitor.c"
    "src/power_management.c"
    "src/resource_monitor.c"
    "src/system_manager.c"
    "src/system_reset.c"
    "src/task_manager.c"
    "src/thermal_management.c"
    "src/watchdog_manager.c"
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
    esp_system
    esp_hw_support
    common             # 共通ユーティリティ
    storage           # ストレージ
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
    SYSTEM_VERSION_MAJOR=1
    SYSTEM_VERSION_MINOR=0
    SYSTEM_VERSION_PATCH=0
    SYSTEM_MONITOR_FREQUENCY_HZ=10     # システム監視頻度
    MEMORY_CHECK_FREQUENCY_HZ=1        # メモリチェック頻度
    WATCHDOG_TIMEOUT_SEC=5            # ウォッチドッグタイムアウト
    ENABLE_PERFORMANCE_MONITORING=1   # パフォーマンス監視有効
    ENABLE_MEMORY_MONITORING=1        # メモリ監視有効
    ENABLE_THERMAL_MONITORING=1       # 温度監視有効
    ENABLE_POWER_MANAGEMENT=1         # 電力管理有効
    MAX_TASK_COUNT=32                 # 最大タスク数
    DIAGNOSTIC_BUFFER_SIZE=512        # 診断バッファサイズ
    FAULT_LOG_SIZE=256               # 障害ログサイズ
)