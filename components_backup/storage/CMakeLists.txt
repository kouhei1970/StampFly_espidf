# Storage（ストレージ管理）コンポーネント CMakeLists.txt
# NVS、SPIFFS、フライトデータレコーダー、設定管理

# コンポーネント情報
set(COMPONENT_NAME "storage")

# ソースファイル
set(COMPONENT_SRCS 
    "src/storage.c"
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
    nvs_flash          # NVS
    spiffs             # SPIFFS
    wear_levelling     # ウェアレベリング
    common            # 共通ユーティリティ
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
    STORAGE_VERSION_MAJOR=1
    STORAGE_VERSION_MINOR=0
    STORAGE_VERSION_PATCH=0
    MAX_FILENAME_LENGTH=64             # 最大ファイル名長
    LOG_BUFFER_SIZE=1024              # ログバッファサイズ
    FLIGHT_DATA_BUFFER_SIZE=2048      # フライトデータバッファサイズ
    AUTO_SAVE_INTERVAL_MS=10000       # 自動保存間隔
    ENABLE_DATA_COMPRESSION=1         # データ圧縮有効
    ENABLE_FLIGHT_DATA_RECORDER=1     # フライトデータレコーダー有効
    ENABLE_BLACK_BOX_RECORDER=1       # ブラックボックスレコーダー有効
    MAX_LOG_FILES=100                 # 最大ログファイル数
    LOG_ROTATION_SIZE_MB=10           # ログローテーションサイズ
)