# Communication（通信システム）コンポーネント CMakeLists.txt
# ESP-NOW、WiFi、Bluetooth、MAVLink、テレメトリー

# コンポーネント情報
set(COMPONENT_NAME "communication")

# ソースファイル
set(COMPONENT_SRCS 
    "src/communication.c"
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
    esp_wifi
    nvs_flash
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
    COMMUNICATION_VERSION_MAJOR=1
    COMMUNICATION_VERSION_MINOR=0
    COMMUNICATION_VERSION_PATCH=0
    ENABLE_ESP_NOW=1                   # ESP-NOW有効
    ENABLE_WIFI=1                      # WiFi有効
    ENABLE_BLUETOOTH=1                 # Bluetooth有効
    ENABLE_MAVLINK=1                   # MAVLink有効
    ESP_NOW_CHANNEL=1                  # ESP-NOWチャンネル
    WIFI_SSID_MAX_LEN=32              # WiFi SSID最大長
    COMM_BUFFER_SIZE=512              # 通信バッファサイズ
    TELEMETRY_FREQUENCY_HZ=50         # テレメトリー頻度
    MAVLINK_SYSTEM_ID=1               # MAVLinkシステムID
    MAVLINK_COMPONENT_ID=1            # MAVLinkコンポーネントID
)