# LED Control（LED制御）コンポーネント CMakeLists.txt
# WS2812 LED制御、アニメーション、ステータス表示

# コンポーネント情報
set(COMPONENT_NAME "led_control")

# ソースファイル
set(COMPONENT_SRCS 
    "src/led_control.c"
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
    driver              # RMT、GPIO
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
)

# プリプロセッサ定義
target_compile_definitions(${COMPONENT_LIB} PRIVATE
    LED_CONTROL_VERSION_MAJOR=1
    LED_CONTROL_VERSION_MINOR=0
    LED_CONTROL_VERSION_PATCH=0
    WS2812_LED_COUNT=1                 # LED数
    WS2812_RMT_CHANNEL=0              # RMTチャンネル
    LED_UPDATE_FREQUENCY_HZ=50         # LED更新頻度
    ENABLE_LED_ANIMATIONS=1            # アニメーション有効
    ENABLE_STATUS_INDICATION=1         # ステータス表示有効
    MAX_BRIGHTNESS=255                 # 最大輝度
    DEFAULT_BRIGHTNESS=128             # デフォルト輝度
    ANIMATION_BUFFER_SIZE=32           # アニメーションバッファサイズ
)