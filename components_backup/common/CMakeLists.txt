# Common（共通ユーティリティ）コンポーネント CMakeLists.txt
# 数学演算、フィルター、汎用ユーティリティ

# コンポーネント情報
set(COMPONENT_NAME "common")

# ソースファイル
set(COMPONENT_SRCS 
    "src/debug_utils.c"
    "src/filters.c"
    "src/interpolation.c"
    "src/math_utils.c"
    "src/matrix_operations.c"
    "src/memory_utils.c"
    "src/numerical_methods.c"
    "src/queue_manager.c"
    "src/ring_buffer.c"
    "src/signal_processing.c"
    "src/statistics.c"
    "src/string_utils.c"
    "src/timer_utils.c"
    "src/vector_math.c"
)

# インクルードディレクトリ（パブリック）
set(COMPONENT_ADD_INCLUDEDIRS 
    "include"
)

# ESP-IDF標準コンポーネント依存関係
set(COMPONENT_REQUIRES
    freertos
    log
    esp_timer
    esp_system
    newlib  # 数学ライブラリ（libm）
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
    -ffast-math      # 数学演算高速化
    -O3              # 最高レベル最適化（数値計算用）
)

# リンクライブラリ
target_link_libraries(${COMPONENT_LIB} PUBLIC
    m  # 数学ライブラリ
)

# プリプロセッサ定義
target_compile_definitions(${COMPONENT_LIB} PRIVATE
    COMMON_COMPONENT_VERSION_MAJOR=1
    COMMON_COMPONENT_VERSION_MINOR=0
    COMMON_COMPONENT_VERSION_PATCH=0
    USE_HARDWARE_FPU=1
    ENABLE_FAST_MATH=1
)