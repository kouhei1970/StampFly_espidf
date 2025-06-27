# HAL（Hardware Abstraction Layer）コンポーネント CMakeLists.txt
# ESP32-S3ハードウェア抽象化層

# コンポーネント情報
set(COMPONENT_NAME "hal")

# ソースファイル
set(COMPONENT_SRCS 
    "src/adc_hal.c"
    "src/dac_hal.c"
    "src/gpio_hal.c"
    "src/i2c_hal.c"
    "src/interrupt_hal.c"
    "src/pwm_hal.c"
    "src/rmt_hal.c"
    "src/spi_hal.c"
    "src/timer_hal.c"
    "src/uart_hal.c"
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
    esp_hw_support
    hal
    soc
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
    -Os  # サイズ最適化
)

# プリプロセッサ定義
target_compile_definitions(${COMPONENT_LIB} PRIVATE
    HAL_COMPONENT_VERSION_MAJOR=1
    HAL_COMPONENT_VERSION_MINOR=0
    HAL_COMPONENT_VERSION_PATCH=0
)