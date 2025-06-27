# Audio（オーディオシステム）コンポーネント CMakeLists.txt
# ブザー制御、トーン生成、メロディー再生、音声通知

# コンポーネント情報
set(COMPONENT_NAME "audio")

# ソースファイル
set(COMPONENT_SRCS 
    "src/audio_system.c"
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
    driver              # PWM、DAC
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
    -Os              # サイズ最適化
)

# リンクライブラリ
target_link_libraries(${COMPONENT_LIB} PUBLIC
    m  # 数学ライブラリ（音声合成用）
)

# プリプロセッサ定義
target_compile_definitions(${COMPONENT_LIB} PRIVATE
    AUDIO_COMPONENT_VERSION_MAJOR=1
    AUDIO_COMPONENT_VERSION_MINOR=0
    AUDIO_COMPONENT_VERSION_PATCH=0
    BUZZER_PWM_FREQUENCY_HZ=2000       # ブザーPWM周波数
    BUZZER_PWM_RESOLUTION_BITS=8       # PWM分解能
    ENABLE_DAC_AUDIO=1                 # DAC音声出力有効
    ENABLE_PWM_AUDIO=1                 # PWM音声出力有効
    AUDIO_SAMPLE_RATE=8000             # サンプリングレート
    MAX_MELODY_LENGTH=256              # 最大メロディー長
    AUDIO_BUFFER_SIZE=128              # 音声バッファサイズ
    ENABLE_AUDIO_NOTIFICATIONS=1       # 音声通知有効
)