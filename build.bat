@echo off
setlocal

set MODE=%1
if "%MODE%"=="" set MODE=debug

set OUTPUT_DIR=build\%MODE%

if not exist "%OUTPUT_DIR%" (
    echo Creating directory: %OUTPUT_DIR%
    mkdir "%OUTPUT_DIR%"
)

set SOURCES=src\*.cpp
set OUTPUT="%OUTPUT_DIR%\Desktop Rain.exe"
set LIBS=-lgdi32 -ld2d1 -ldwrite -lole32 -luuid -lcomdlg32 -lshell32
set RES_OBJ=

if exist resources.rc (
    echo Compiling resources...
    windres resources.rc -o resources.o
    set RES_OBJ=resources.o
)

if /I "%MODE%"=="release" (
    set FLAGS=-O3 -std=c++23 -mwindows -municode -s -DNDEBUG -DUNICODE -D_UNICODE
    echo Building Release...
) else (
    set FLAGS=-g -std=c++23 -Wall -mwindows -municode -DUNICODE -D_UNICODE
    echo Building Debug...
)

echo g++ %SOURCES% %RES_OBJ% -o %OUTPUT% %FLAGS% %LIBS%
g++ %SOURCES% %RES_OBJ% -o %OUTPUT% %FLAGS% %LIBS%
set BUILD_STATUS=%ERRORLEVEL%

if exist resources.o (
    del resources.o
)

if %BUILD_STATUS%==0 (
    echo Build successful: %OUTPUT%
) else (
    echo Build failed!
    exit /b 1
)

endlocal
