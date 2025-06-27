# CLI Interface（コマンドラインインターフェース）コンポーネント CMakeLists.txt
# USB CDC CLI、コマンド解析、データストリーミング

# コンポーネント情報
set(COMPONENT_NAME "cli_interface")

# ソースファイル
set(COMPONENT_SRCS 
    "src/cli_interface.c"
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
    nvs_flash           # パラメータ保存
    sensor_fusion       # センサーデータ
    attitude_estimation # 姿勢データ
    position_estimation # 位置データ
    control_manager     # 制御データ
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
    CLI_INTERFACE_VERSION_MAJOR=1
    CLI_INTERFACE_VERSION_MINOR=0
    CLI_INTERFACE_VERSION_PATCH=0
    CLI_TASK_STACK_SIZE=4096           # CLIタスクスタックサイズ
    CLI_TASK_PRIORITY=3                # CLIタスク優先度
    CLI_RX_BUFFER_SIZE=512             # 受信バッファサイズ
    CLI_TX_BUFFER_SIZE=1024            # 送信バッファサイズ
    CLI_HISTORY_SIZE=10                # コマンド履歴サイズ
    CLI_MAX_COMMAND_LENGTH=128         # 最大コマンド長
    CLI_MAX_ARGS=16                    # 最大引数数
    ENABLE_CLI_COMPLETION=1            # オートコンプリート有効
    ENABLE_DATA_STREAMING=1            # データストリーミング有効
    STREAMING_MAX_FREQUENCY_HZ=1000    # 最大ストリーミング頻度
)