# Attitude Estimation（姿勢推定）コンポーネント CMakeLists.txt
# AHRS、クォータニオン、姿勢角演算

# コンポーネント情報
set(COMPONENT_NAME "attitude_estimation")

# ソースファイル
set(COMPONENT_SRCS 
    "src/attitude_estimation.c"
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
    sensor_fusion    # センサー融合
    common          # 共通ユーティリティ
    newlib          # 数学ライブラリ
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
    -O3              # 最高レベル最適化（姿勢推定は計算集約的）
    -ffast-math      # 数学演算最適化
    -funroll-loops   # ループ最適化
)

# リンクライブラリ
target_link_libraries(${COMPONENT_LIB} PUBLIC
    m  # 数学ライブラリ
)

# プリプロセッサ定義
target_compile_definitions(${COMPONENT_LIB} PRIVATE
    ATTITUDE_ESTIMATION_VERSION_MAJOR=1
    ATTITUDE_ESTIMATION_VERSION_MINOR=0
    ATTITUDE_ESTIMATION_VERSION_PATCH=0
    USE_MADGWICK_AHRS=1
    USE_EXTENDED_KALMAN_FILTER=1
    ATTITUDE_UPDATE_FREQUENCY_HZ=400
    ENABLE_QUATERNION_NORMALIZATION=1
    GYRO_INTEGRATION_METHOD=2  # 高精度積分
)