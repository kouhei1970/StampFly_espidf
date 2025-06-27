# HAL Component CMakeLists.txt
# 
# 作成者: Kouhei Ito
# ライセンス: MIT License
# 
# Copyright (c) 2025 Kouhei Ito

idf_component_register(
    SRCS 
        "src/hal_base.cpp"
        "src/gpio_hal.cpp"
        "src/i2c_hal.cpp"
        "src/spi_hal.cpp"
    INCLUDE_DIRS 
        "include"
    REQUIRES 
        "driver" 
        "esp_timer"
        "esp_common"
        "freertos"
        "log"
)