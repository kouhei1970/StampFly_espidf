# StampFly メインアプリケーション CMakeLists.txt

# メインアプリケーションのソースファイル
set(MAIN_SRCS 
    "main.c"
)

# インクルードディレクトリ
set(MAIN_INCLUDE_DIRS 
    "."
)

# コンポーネント依存関係（HAL一時除外）
set(MAIN_REQUIRES
    freertos
    log
    esp_timer
)

# コンポーネント登録
idf_component_register(
    SRCS ${MAIN_SRCS}
    INCLUDE_DIRS ${MAIN_INCLUDE_DIRS}
    REQUIRES ${MAIN_REQUIRES}
)