# Button Control（ボタン制御）コンポーネント CMakeLists.txt
# ボタン入力処理、マルチプレス検出、ジェスチャー認識

# コンポーネント情報
set(COMPONENT_NAME "button_control")

# ソースファイル
set(COMPONENT_SRCS 
    "src/button_control.c"
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
    driver              # GPIO
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

# プリプロセッサ定義
target_compile_definitions(${COMPONENT_LIB} PRIVATE
    BUTTON_CONTROL_VERSION_MAJOR=1
    BUTTON_CONTROL_VERSION_MINOR=0
    BUTTON_CONTROL_VERSION_PATCH=0
    BUTTON_DEBOUNCE_TIME_MS=50         # チャタリング除去時間
    BUTTON_LONG_PRESS_TIME_MS=1000     # 長押し判定時間
    BUTTON_DOUBLE_CLICK_TIME_MS=300    # ダブルクリック判定時間
    MAX_BUTTON_COUNT=4                 # 最大ボタン数
    ENABLE_MULTI_PRESS_DETECTION=1     # マルチプレス検出有効
    ENABLE_GESTURE_RECOGNITION=1       # ジェスチャー認識有効
    BUTTON_SCAN_FREQUENCY_HZ=100       # ボタンスキャン頻度
)