# Sensors（センサー統合）コンポーネント CMakeLists.txt
# 全センサードライバーの統合インターフェース

# コンポーネント情報
set(COMPONENT_NAME "sensors")

# ソースファイル
set(COMPONENT_SRCS 
    "src/sensor_manager.c"
    "src/sensor_interface.c"
    "src/sensor_types.c"
)

# インクルードディレクトリ（パブリック）
set(COMPONENT_ADD_INCLUDEDIRS 
    "include"
)

# ESP-IDF標準コンポーネント依存関係
set(COMPONENT_REQUIRES
    driver
    esp_timer
    freertos
    log
    hal          # カスタムHALコンポーネント
    common       # カスタム共通ユーティリティ
)

# TODO: サブディレクトリのソースファイルを追加
# file(GLOB_RECURSE BMI270_SRCS "bmi270/src/*.c")
# file(GLOB_RECURSE BMM150_SRCS "bmm150/src/*.c")
# file(GLOB_RECURSE VL53L3CX_SRCS "vl53l3cx/src/*.c")
# file(GLOB_RECURSE BMP280_SRCS "bmp280/src/*.c")
# file(GLOB_RECURSE INA3221_SRCS "ina3221/src/*.c")
# file(GLOB_RECURSE OPTICAL_FLOW_SRCS "optical_flow/src/*.c")

# TODO: 全てのセンサーソースを統合
# list(APPEND COMPONENT_SRCS 
#     ${BMI270_SRCS}
#     ${BMM150_SRCS}
#     ${VL53L3CX_SRCS}
#     ${BMP280_SRCS}
#     ${INA3221_SRCS}
#     ${OPTICAL_FLOW_SRCS}
# )

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
    -Os  # サイズ最適化
)

# プリプロセッサ定義
target_compile_definitions(${COMPONENT_LIB} PRIVATE
    SENSORS_COMPONENT_VERSION_MAJOR=1
    SENSORS_COMPONENT_VERSION_MINOR=0
    SENSORS_COMPONENT_VERSION_PATCH=0
    BMI270_ENABLED=1
    BMM150_ENABLED=1
    VL53L3CX_ENABLED=1
    BMP280_ENABLED=1
    INA3221_ENABLED=1
    OPTICAL_FLOW_ENABLED=1
)