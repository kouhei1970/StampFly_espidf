# Remote Control（リモートコントロール）コンポーネント CMakeLists.txt
# ATOMJoy制御、RC信号処理、フェイルセーフ

# コンポーネント情報
set(COMPONENT_NAME "remote_control")

# ソースファイル
set(COMPONENT_SRCS 
    "src/remote_control.c"
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
    nvs_flash           # 設定保存
    communication       # ESP-NOW通信
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
    REMOTE_CONTROL_VERSION_MAJOR=1
    REMOTE_CONTROL_VERSION_MINOR=0
    REMOTE_CONTROL_VERSION_PATCH=0
    RC_CHANNELS=8                      # RCチャンネル数
    RC_UPDATE_FREQUENCY_HZ=100         # RC更新頻度
    RC_TIMEOUT_MS=1000                # RCタイムアウト
    ENABLE_RC_FAILSAFE=1              # フェイルセーフ有効
    ENABLE_MANUAL_OVERRIDE=1          # 手動オーバーライド有効
    STICK_DEADBAND=10                 # スティックデッドバンド
    STICK_EXPO=0.3                    # スティックエキスポ
    RC_FILTER_ALPHA=0.8               # RCフィルター係数
    ATOMJOY_PACKET_SIZE=32            # ATOMJoyパケットサイズ
)