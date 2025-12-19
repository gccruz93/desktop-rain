#!/bin/bash

MODE=${1:-debug}

OUTPUT_DIR="build/$MODE"

if [ ! -d "$OUTPUT_DIR" ]; then
    echo "Creating directory: $OUTPUT_DIR"
    mkdir -p "$OUTPUT_DIR"
fi

SOURCES="src/*.cpp"
OUTPUT="$OUTPUT_DIR/Desktop Rain.exe"
LIBS="-lgdi32 -ld2d1 -lole32 -luuid -lcomdlg32 -lshell32"
RES_OBJ=""

if [ -f "resources.rc" ]; then
    echo "Compiling resources..."
    windres resources.rc -o resources.o
    RES_OBJ="resources.o"
fi

if [ "$MODE" == "release" ]; then
    FLAGS="-O2 -std=c++20 -mwindows -s -DNDEBUG"
    echo "Building Release configuration..."
else
    FLAGS="-g -std=c++20 -Wall"
    echo "Building Debug configuration..."
fi

g++ $SOURCES $RES_OBJ -o $OUTPUT $FLAGS $LIBS

BUILD_STATUS=$?

if [ -f "resources.o" ]; then
    rm resources.o
fi

if [ $BUILD_STATUS -eq 0 ]; then
    echo "Build successful: $OUTPUT"
else
    echo "Build failed!"
    exit 1
fi
