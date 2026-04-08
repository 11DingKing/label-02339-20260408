cmake_minimum_required(VERSION 3.16)
project(EnvMonitor VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets SerialPort Network Sql Test)

# ==================== 自动收集源文件 ====================
file(GLOB_RECURSE LIB_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/**/*.cpp
)

file(GLOB_RECURSE LIB_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/**/*.h
)

# 排除 main.cpp，因为它是主程序入口
list(REMOVE_ITEM LIB_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp)

set(RESOURCES
    resources/resources.qrc
)

# ==================== 主程序库（供测试复用） ====================
add_library(envmonitor_lib STATIC ${LIB_SOURCES} ${LIB_HEADERS} ${RESOURCES})
target_link_libraries(envmonitor_lib PUBLIC
    Qt6::Core Qt6::Gui Qt6::Widgets Qt6::SerialPort Qt6::Network Qt6::Sql
)
target_include_directories(envmonitor_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

# ==================== 主程序可执行文件 ====================
add_executable(${PROJECT_NAME} src/main.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE envmonitor_lib)

# ==================== 自动收集测试文件 ====================
file(GLOB_RECURSE TEST_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_*.cpp
)

# ==================== 测试 ====================
enable_testing()

# 为每个测试文件创建测试可执行文件
foreach(TEST_SOURCE ${TEST_SOURCES})
    get_filename_component(TEST_NAME ${TEST_SOURCE} NAME_WE)
    add_executable(${TEST_NAME} ${TEST_SOURCE})
    target_link_libraries(${TEST_NAME} PRIVATE envmonitor_lib Qt6::Test)
    
    # 为 GUI 测试添加额外的 Qt 组件
    if(TEST_NAME STREQUAL "test_gaugewidget")
        target_link_libraries(${TEST_NAME} PRIVATE Qt6::Gui Qt6::Widgets)
    endif()
    
    # 生成测试名称（去掉 test_ 前缀，首字母大写）
    string(REGEX REPLACE "^test_" "" TEST_DISPLAY_NAME ${TEST_NAME})
    string(SUBSTRING ${TEST_DISPLAY_NAME} 0 1 FIRST_CHAR)
    string(TOUPPER ${FIRST_CHAR} FIRST_CHAR_UPPER)
    string(REGEX REPLACE "^." ${FIRST_CHAR_UPPER} TEST_DISPLAY_NAME ${TEST_DISPLAY_NAME})
    
    add_test(NAME ${TEST_DISPLAY_NAME} COMMAND ${TEST_NAME})
endforeach()

# ==================== CPack 打包配置 ====================
set(CPACK_PACKAGE_NAME ${PROJECT_NAME})
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "环境监测系统")
set(CPACK_PACKAGE_VENDOR "EnvMonitor Team")
set(CPACK_PACKAGE_CONTACT "contact@envmonitor.example.com")
set(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/../README.md")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/../LICENSE")

# macOS 特定配置
if(APPLE)
    set(CPACK_GENERATOR "DragNDrop")
    set(CPACK_DMG_VOLUME_NAME "${PROJECT_NAME} ${PROJECT_VERSION}")
    set(CPACK_DMG_FORMAT "UDBZ")
    set(CPACK_DMG_DS_STORE_SETUP_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/cmake/dmg_setup.scpt")
    set(CPACK_DMG_BACKGROUND_IMAGE "${CMAKE_CURRENT_SOURCE_DIR}/resources/dmg_background.png")
    
    # 设置 macOS 应用程序包信息
    set_target_properties(${PROJECT_NAME} PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/cmake/Info.plist.in"
        MACOSX_BUNDLE_BUNDLE_NAME ${PROJECT_NAME}
        MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
        MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION}
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.envmonitor.${PROJECT_NAME}"
        MACOSX_BUNDLE_ICON_FILE "${PROJECT_NAME}.icns"
    )
    
    # 安装应用程序包
    install(TARGETS ${PROJECT_NAME}
        BUNDLE DESTINATION .
        RUNTIME DESTINATION bin
    )
    
    # 使用 macdeployqt 部署 Qt 依赖
    find_program(MACDEPLOYQT_EXECUTABLE macdeployqt HINTS "${Qt6_DIR}/../../../bin")
    if(MACDEPLOYQT_EXECUTABLE)
        install(CODE "
            execute_process(
                COMMAND \"${MACDEPLOYQT_EXECUTABLE}\" \"\${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}.app\"
                RESULT_VARIABLE MACDEPLOYQT_RESULT
            )
            if(NOT MACDEPLOYQT_RESULT EQUAL 0)
                message(FATAL_ERROR \"macdeployqt failed\")
            endif()
        ")
    endif()
endif()

# Linux 特定配置
if(UNIX AND NOT APPLE)
    set(CPACK_GENERATOR "DEB")
    set(CPACK_DEBIAN_PACKAGE_NAME ${PROJECT_NAME})
    set(CPACK_DEBIAN_PACKAGE_VERSION ${PROJECT_VERSION})
    set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "amd64")
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "libqt6core6, libqt6gui6, libqt6widgets6, libqt6serialport6, libqt6network6, libqt6sql6")
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER ${CPACK_PACKAGE_CONTACT})
    set(CPACK_DEBIAN_PACKAGE_DESCRIPTION "环境监测系统\n 一个用于监测环境参数的 Qt 应用程序。")
    set(CPACK_DEBIAN_PACKAGE_SECTION "utils")
    set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
    
    # 安装可执行文件
    install(TARGETS ${PROJECT_NAME}
        RUNTIME DESTINATION bin
    )
    
    # 安装桌面文件
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/cmake/${PROJECT_NAME}.desktop"
        DESTINATION share/applications
    )
    
    # 安装图标
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/resources/icons/${PROJECT_NAME}.png"
        DESTINATION share/icons/hicolor/128x128/apps
    )
endif()

include(CPack)
