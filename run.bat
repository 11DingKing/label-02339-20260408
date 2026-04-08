@echo off
chcp 65001 >nul
setlocal EnableDelayedExpansion

:: 环境监测系统 - Windows 一键启动脚本
:: 支持自动检测和安装依赖

title 环境监测系统 - EnvMonitor

echo.
echo ==========================================
echo   环境监测系统 - 一键启动
echo ==========================================
echo.

:: 设置路径
set "SCRIPT_DIR=%~dp0"
set "BACKEND_DIR=%SCRIPT_DIR%backend"
set "BUILD_DIR=%BACKEND_DIR%\build"

:: 检查 Qt 安装
call :check_qt
if errorlevel 1 goto :install_guide

:: 检查 CMake
call :check_cmake
if errorlevel 1 goto :install_guide

:: 编译项目
if not exist "%BUILD_DIR%\EnvMonitor.exe" (
    call :build_project
) else (
    if "%1"=="--rebuild" (
        call :build_project
    ) else (
        echo [信息] 检测到已编译的程序，跳过编译
        echo        使用 run.bat --rebuild 强制重新编译
    )
)

echo.

:: 初始化数据库
call :init_database

echo.

:: 启动测试服务器
call :start_test_server

echo.

:: 运行程序
call :run_app

goto :eof

:: ==================== 函数定义 ====================

:check_qt
echo [检查] 正在检测 Qt6 安装...

:: 检查常见 Qt 安装路径
set "QT_PATHS=C:\Qt\6.6.0\msvc2019_64;C:\Qt\6.5.0\msvc2019_64;C:\Qt\6.4.0\msvc2019_64"
set "QT_PATHS=%QT_PATHS%;C:\Qt\6.6.0\mingw_64;C:\Qt\6.5.0\mingw_64"
set "QT_PATHS=%QT_PATHS%;D:\Qt\6.6.0\msvc2019_64;D:\Qt\6.5.0\msvc2019_64"

for %%p in (%QT_PATHS%) do (
    if exist "%%p\bin\qmake.exe" (
        set "QT_PATH=%%p"
        echo [成功] 找到 Qt6: %%p
        exit /b 0
    )
)

:: 检查环境变量
if defined Qt6_DIR (
    if exist "%Qt6_DIR%\bin\qmake.exe" (
        set "QT_PATH=%Qt6_DIR%"
        echo [成功] 找到 Qt6: %Qt6_DIR%
        exit /b 0
    )
)

:: 检查 PATH 中的 qmake
where qmake >nul 2>&1
if %errorlevel%==0 (
    for /f "tokens=*" %%i in ('where qmake') do (
        set "QMAKE_PATH=%%i"
        for %%a in ("!QMAKE_PATH!") do set "QT_PATH=%%~dpa.."
        echo [成功] 找到 Qt6: !QT_PATH!
        exit /b 0
    )
)

echo [错误] 未找到 Qt6 安装
exit /b 1

:check_cmake
echo [检查] 正在检测 CMake...

where cmake >nul 2>&1
if %errorlevel%==0 (
    echo [成功] CMake 已安装
    exit /b 0
)

:: 检查常见安装路径
if exist "C:\Program Files\CMake\bin\cmake.exe" (
    set "PATH=%PATH%;C:\Program Files\CMake\bin"
    echo [成功] CMake 已安装
    exit /b 0
)

echo [错误] 未找到 CMake
exit /b 1

:install_guide
echo.
echo ==========================================
echo   需要安装以下依赖
echo ==========================================
echo.
echo 1. Qt6 (推荐 6.5.0 或更高版本)
echo    下载地址: https://www.qt.io/download-qt-installer
echo    安装时选择: Qt 6.x.x ^> MSVC 2019 64-bit 或 MinGW
echo.
echo 2. CMake (3.16 或更高版本)
echo    下载地址: https://cmake.org/download/
echo.
echo 3. 编译器
echo    - Visual Studio 2019/2022 (推荐)
echo    - 或 MinGW-w64
echo.
echo 安装完成后，请重新运行此脚本。
echo.
pause
exit /b 1

:build_project
echo.
echo [编译] 开始编译项目...

:: 创建构建目录
if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"
cd /d "%BUILD_DIR%"

:: CMake 配置
echo [编译] 运行 CMake 配置...
cmake .. -DCMAKE_PREFIX_PATH="%QT_PATH%" -DCMAKE_BUILD_TYPE=Release
if errorlevel 1 (
    echo [错误] CMake 配置失败
    pause
    exit /b 1
)

:: 编译
echo [编译] 编译中...
cmake --build . --config Release --parallel
if errorlevel 1 (
    echo [错误] 编译失败
    pause
    exit /b 1
)

echo [成功] 编译完成
exit /b 0

:init_database
set "DB_PATH=%BUILD_DIR%\envmonitor.db"
set "SCHEMA_FILE=%BACKEND_DIR%\schema.sql"
set "DATA_FILE=%BACKEND_DIR%\init_data.sql"

if exist "%DB_PATH%" (
    echo [信息] 数据库已存在
    exit /b 0
)

echo [初始化] 创建数据库...

:: 检查 sqlite3
where sqlite3 >nul 2>&1
if errorlevel 1 (
    echo [警告] 未找到 sqlite3，跳过数据库初始化
    echo        程序启动后会自动创建数据库
    exit /b 0
)

if exist "%SCHEMA_FILE%" (
    sqlite3 "%DB_PATH%" < "%SCHEMA_FILE%"
    echo [成功] 数据库表结构已创建
)

if exist "%DATA_FILE%" (
    sqlite3 "%DB_PATH%" < "%DATA_FILE%"
    echo [成功] 测试数据已导入
)

exit /b 0

:start_test_server
set "SERVER_SCRIPT=%SCRIPT_DIR%test_server.py"

if not exist "%SERVER_SCRIPT%" (
    echo [警告] 测试服务器脚本不存在，跳过
    exit /b 0
)

:: 检查 Python
where python >nul 2>&1
if errorlevel 1 (
    where python3 >nul 2>&1
    if errorlevel 1 (
        echo [警告] 未检测到 Python，跳过测试服务器
        echo        如需测试网络上传，请安装 Python 后手动运行: python test_server.py
        exit /b 0
    )
    set "PYTHON_CMD=python3"
) else (
    set "PYTHON_CMD=python"
)

echo [启动] 启动测试服务器（端口 9000）...
start "测试服务器" /min %PYTHON_CMD% "%SERVER_SCRIPT%" 9000
echo [成功] 测试服务器已启动
echo [信息] 网络上传测试: 服务器地址 127.0.0.1，端口 9000

exit /b 0

:run_app
set "APP_PATH=%BUILD_DIR%\Release\EnvMonitor.exe"
if not exist "%APP_PATH%" (
    set "APP_PATH=%BUILD_DIR%\EnvMonitor.exe"
)

if not exist "%APP_PATH%" (
    echo [错误] 找不到可执行文件
    pause
    exit /b 1
)

echo.
echo ==========================================
echo   环境监测系统 EnvMonitor v1.0
echo ==========================================
echo   测试服务器: 127.0.0.1:9000 (TCP/UDP)
echo ==========================================
echo.
echo [启动] 正在启动程序...

:: 添加 Qt DLL 路径
set "PATH=%QT_PATH%\bin;%PATH%"

cd /d "%BUILD_DIR%"
start "" "%APP_PATH%"

exit /b 0
