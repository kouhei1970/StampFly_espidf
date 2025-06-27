# PID Control（PID制御）コンポーネント CMakeLists.txt
# PID制御システム、チューニング、適応制御

# コンポーネント情報
set(COMPONENT_NAME "pid_control")

# ソースファイル
set(COMPONENT_SRCS 
    "src/pid_control.c"
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
    attitude_estimation  # 姿勢推定
    position_estimation  # 位置推定
    common              # 共通ユーティリティ
    nvs_flash           # パラメータ保存
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
    PID_CONTROL_VERSION_MAJOR=1
    PID_CONTROL_VERSION_MINOR=0
    PID_CONTROL_VERSION_PATCH=0
    PID_CONTROL_FREQUENCY_HZ=400
    ENABLE_ADAPTIVE_PID=1
    ENABLE_GAIN_SCHEDULING=1
    ENABLE_ANTI_WINDUP=1
    ENABLE_DERIVATIVE_FILTER=1
    MAX_PID_CONTROLLERS=8
    PID_HISTORY_SIZE=10
)