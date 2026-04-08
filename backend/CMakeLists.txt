cmake_minimum_required(VERSION 3.16)
project(EnvMonitor VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets SerialPort Network Sql Test)

# ==================== 主程序库（供测试复用） ====================
set(LIB_SOURCES
    src/mainwindow.cpp
    src/dialogs/settingsdialog.cpp
    src/managers/serialmanager.cpp
    src/managers/networkmanager.cpp
    src/managers/databasemanager.cpp
    src/managers/configmanager.cpp
    src/core/dataprocessor.cpp
    src/models/sensordata.cpp
    src/widgets/gaugewidget.cpp
    src/utils/stringutils.cpp
    src/utils/logmanager.cpp
)

set(LIB_HEADERS
    src/mainwindow.h
    src/dialogs/settingsdialog.h
    src/managers/serialmanager.h
    src/managers/networkmanager.h
    src/managers/databasemanager.h
    src/managers/configmanager.h
    src/core/dataprocessor.h
    src/models/sensordata.h
    src/widgets/gaugewidget.h
    src/utils/stringutils.h
    src/utils/logmanager.h
)

set(RESOURCES
    resources/resources.qrc
)

add_library(envmonitor_lib STATIC ${LIB_SOURCES} ${LIB_HEADERS} ${RESOURCES})
target_link_libraries(envmonitor_lib PUBLIC
    Qt6::Core Qt6::Gui Qt6::Widgets Qt6::SerialPort Qt6::Network Qt6::Sql
)
target_include_directories(envmonitor_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

# ==================== 主程序可执行文件 ====================
add_executable(${PROJECT_NAME} src/main.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE envmonitor_lib)

# ==================== 测试 ====================
enable_testing()

# 测试: SensorData 模型
add_executable(test_sensordata tests/test_sensordata.cpp)
target_link_libraries(test_sensordata PRIVATE envmonitor_lib Qt6::Test)
add_test(NAME TestSensorData COMMAND test_sensordata)

# 测试: DatabaseManager
add_executable(test_database tests/test_database.cpp)
target_link_libraries(test_database PRIVATE envmonitor_lib Qt6::Test)
add_test(NAME TestDatabase COMMAND test_database)

# 测试: ConfigManager
add_executable(test_config tests/test_config.cpp)
target_link_libraries(test_config PRIVATE envmonitor_lib Qt6::Test)
add_test(NAME TestConfig COMMAND test_config)

# 测试: NetworkManager
add_executable(test_network tests/test_network.cpp)
target_link_libraries(test_network PRIVATE envmonitor_lib Qt6::Test)
add_test(NAME TestNetwork COMMAND test_network)

# 测试: SerialManager (模拟模式)
add_executable(test_serial tests/test_serial.cpp)
target_link_libraries(test_serial PRIVATE envmonitor_lib Qt6::Test)
add_test(NAME TestSerial COMMAND test_serial)

# 测试: DataProcessor
add_executable(test_dataprocessor tests/test_dataprocessor.cpp)
target_link_libraries(test_dataprocessor PRIVATE envmonitor_lib Qt6::Test)
add_test(NAME TestDataProcessor COMMAND test_dataprocessor)

# 测试: GaugeWidget
add_executable(test_gaugewidget tests/test_gaugewidget.cpp)
target_link_libraries(test_gaugewidget PRIVATE envmonitor_lib Qt6::Test Qt6::Gui Qt6::Widgets)
add_test(NAME TestGaugeWidget COMMAND test_gaugewidget)

# 集成测试
add_executable(test_integration tests/test_integration.cpp)
target_link_libraries(test_integration PRIVATE envmonitor_lib Qt6::Test)
add_test(NAME TestIntegration COMMAND test_integration)
