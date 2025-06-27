# StampFly ESP-IDF版 メインCMakeファイル
# ESP32-S3ベースドローン制御システム
# 作成者: Kouhei Ito
# ライセンス: MIT

cmake_minimum_required(VERSION 3.16)

# ESP-IDFコンポーネントを含む
include($ENV{IDF_PATH}/tools/cmake/project.cmake)

# プロジェクト定義
project(stampfly_espidf)