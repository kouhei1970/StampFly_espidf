# Position Estimation（位置推定）コンポーネント CMakeLists.txt
# カルマンフィルター、高度推定、位置推定

# コンポーネント情報
set(COMPONENT_NAME "position_estimation")

# ソースファイル
set(COMPONENT_SRCS 
    "src/position_estimation.c"
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
    sensor_fusion       # センサー融合
    common             # 共通ユーティリティ
    newlib             # 数学ライブラリ
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
    -O3              # 最高レベル最適化
    -ffast-math      # 数学演算最適化
    -funroll-loops   # ループ最適化
)

# リンクライブラリ
target_link_libraries(${COMPONENT_LIB} PUBLIC
    m  # 数学ライブラリ
)

# プリプロセッサ定義
target_compile_definitions(${COMPONENT_LIB} PRIVATE
    POSITION_ESTIMATION_VERSION_MAJOR=1
    POSITION_ESTIMATION_VERSION_MINOR=0
    POSITION_ESTIMATION_VERSION_PATCH=0
    USE_KALMAN_FILTER=1
    USE_PARTICLE_FILTER=1
    POSITION_UPDATE_FREQUENCY_HZ=100
    ALTITUDE_UPDATE_FREQUENCY_HZ=400
    ENABLE_OPTICAL_FLOW=1
    ENABLE_VISUAL_ODOMETRY=1
    KALMAN_STATE_SIZE=6      # [x, y, z, vx, vy, vz]
    ALTITUDE_STATE_SIZE=2    # [altitude, velocity]
)