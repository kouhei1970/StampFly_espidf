# Navigation（ナビゲーション）コンポーネント CMakeLists.txt
# ウェイポイント、軌道生成、障害物回避、自動機能

# コンポーネント情報
set(COMPONENT_NAME "navigation")

# ソースファイル
set(COMPONENT_SRCS 
    "src/navigation.c"
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
    position_estimation # 位置推定
    attitude_estimation # 姿勢推定
    control_manager     # 制御管理
    common             # 共通ユーティリティ
    nvs_flash          # 設定保存
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
    -ffast-math      # 数学演算最適化
)

# リンクライブラリ
target_link_libraries(${COMPONENT_LIB} PUBLIC
    m  # 数学ライブラリ
)

# プリプロセッサ定義
target_compile_definitions(${COMPONENT_LIB} PRIVATE
    NAVIGATION_VERSION_MAJOR=1
    NAVIGATION_VERSION_MINOR=0
    NAVIGATION_VERSION_PATCH=0
    NAV_UPDATE_FREQUENCY_HZ=50         # ナビゲーション更新頻度
    MAX_WAYPOINTS=32                   # 最大ウェイポイント数
    TRAJECTORY_RESOLUTION=0.1          # 軌道分解能（m）
    OBSTACLE_DETECTION_RANGE=2.0       # 障害物検出範囲（m）
    GEOFENCE_RADIUS=50.0              # ジオフェンス半径（m）
    AUTO_LANDING_SPEED=0.5            # 自動着陸速度（m/s）
    AUTO_TAKEOFF_HEIGHT=1.0           # 自動離陸高度（m）
    ENABLE_OBSTACLE_AVOIDANCE=1       # 障害物回避有効
    ENABLE_GEOFENCING=1               # ジオフェンス有効
    ENABLE_RETURN_TO_HOME=1           # ホーム復帰有効
)