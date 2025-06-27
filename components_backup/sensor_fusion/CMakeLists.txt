# Sensor Fusion（センサー融合）コンポーネント CMakeLists.txt
# 複数センサーデータの統合・融合処理

# コンポーネント情報
set(COMPONENT_NAME "sensor_fusion")

# ソースファイル
set(COMPONENT_SRCS 
    "src/sensor_fusion.c"
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
    sensors      # センサーコンポーネント
    common       # 共通ユーティリティ
    hal          # HAL層
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

# プリプロセッサ定義
target_compile_definitions(${COMPONENT_LIB} PRIVATE
    SENSOR_FUSION_VERSION_MAJOR=1
    SENSOR_FUSION_VERSION_MINOR=0
    SENSOR_FUSION_VERSION_PATCH=0
    ENABLE_SENSOR_HEALTH_CHECK=1
    ENABLE_OUTLIER_DETECTION=1
    SENSOR_FUSION_FREQUENCY_HZ=1000
    DATA_FUSION_BUFFER_SIZE=128
)