# MPC Control（モデル予測制御）コンポーネント CMakeLists.txt
# MPC制御システム、最適化ソルバー、予測モデル

# コンポーネント情報
set(COMPONENT_NAME "mpc_control")

# ソースファイル
set(COMPONENT_SRCS 
    "src/mpc_control.c"
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
    attitude_estimation  # 姿勢推定
    position_estimation  # 位置推定
    common              # 共通ユーティリティ
    nvs_flash           # パラメータ保存
    newlib              # 数学ライブラリ
)

# コンポーネント登録
idf_component_register(
    SRCS ${COMPONENT_SRCS}
    INCLUDE_DIRS ${COMPONENT_ADD_INCLUDEDIRS}
    REQUIRES ${COMPONENT_REQUIRES}
)

# コンパイルオプション（MPC は計算集約的なので最高レベル最適化）
target_compile_options(${COMPONENT_LIB} PRIVATE
    -Wall
    -Wextra
    -Werror=return-type
    -O3              # 最高レベル最適化
    -ffast-math      # 数学演算最適化
    -funroll-loops   # ループ最適化
    -finline-functions # 関数インライン化
)

# リンクライブラリ
target_link_libraries(${COMPONENT_LIB} PUBLIC
    m  # 数学ライブラリ
)

# プリプロセッサ定義
target_compile_definitions(${COMPONENT_LIB} PRIVATE
    MPC_CONTROL_VERSION_MAJOR=1
    MPC_CONTROL_VERSION_MINOR=0
    MPC_CONTROL_VERSION_PATCH=0
    MPC_CONTROL_FREQUENCY_HZ=100   # MPCは低頻度実行
    MPC_PREDICTION_HORIZON=10      # 予測ホライゾン
    MPC_CONTROL_HORIZON=3          # 制御ホライゾン
    MPC_STATE_SIZE=12              # 状態変数数 [位置、速度、姿勢、角速度]
    MPC_INPUT_SIZE=4               # 入力変数数 [4つのモーター]
    ENABLE_ROBUST_MPC=1
    ENABLE_ADAPTIVE_MPC=1
    ENABLE_NONLINEAR_MPC=1
    USE_SPARSE_MATRICES=1
    MAX_OPTIMIZATION_ITERATIONS=50
)