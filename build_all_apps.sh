#!/usr/bin/env bash

set -u

# Configuration
ROOT_DIR="$(pwd)"
APPS_DIR="$ROOT_DIR/apps"
MYTEST_DIR="$ROOT_DIR/examples/mytest"
MYTEST_INO="$MYTEST_DIR/mytest.ino"
BINARIES_DIR="$ROOT_DIR/binaries"
LIBRARIES_DIR="$ROOT_DIR/libraries"
BUILD_DIR="$MYTEST_DIR/build"
ARDUINO_CLI_CONFIG="$ROOT_DIR/arduino-cli.yaml"

if [ -n "${BOARD_FQBN:-}" ]; then
    BOARD_FQBN="$BOARD_FQBN"
elif [ -f "$ROOT_DIR/boards.fqbn" ]; then
    BOARD_FQBN="$(head -n 1 "$ROOT_DIR/boards.fqbn" | tr -d '\r')"
else
    BOARD_FQBN="esp32:esp32:esp32"
fi

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Create binaries directory if it doesn't exist
mkdir -p "$BINARIES_DIR"

if ! command -v arduino-cli > /dev/null 2>&1; then
    echo -e "${RED}✗ arduino-cli not found in PATH${NC}"
    exit 1
fi

if [ ! -d "$LIBRARIES_DIR" ]; then
    echo -e "${RED}✗ Libraries directory not found: $LIBRARIES_DIR${NC}"
    exit 1
fi

if [ ! -f "$MYTEST_INO" ]; then
    echo -e "${RED}✗ Sketch file not found: $MYTEST_INO${NC}"
    exit 1
fi

CLI_ARGS=()
if [ -f "$ARDUINO_CLI_CONFIG" ]; then
    CLI_ARGS+=(--config-file "$ARDUINO_CLI_CONFIG")
fi

if ! arduino-cli "${CLI_ARGS[@]}" board details --fqbn "$BOARD_FQBN" > /dev/null 2>&1; then
    echo -e "${RED}✗ Board FQBN is not available: $BOARD_FQBN${NC}"
    echo "  Check $ROOT_DIR/boards.fqbn and installed cores."
    exit 1
fi

ORIGINAL_MYTEST_BACKUP="$(mktemp)"
cp "$MYTEST_INO" "$ORIGINAL_MYTEST_BACKUP"
cleanup() {
    cp "$ORIGINAL_MYTEST_BACKUP" "$MYTEST_INO"
    rm -f "$ORIGINAL_MYTEST_BACKUP"
    rm -rf "$BUILD_DIR"
}
trap cleanup EXIT

# Keep track of successes and failures
SUCCESS_COUNT=0
FAIL_COUNT=0
FAILED_APPS=()

echo "========================================="
echo "Building all apps from $APPS_DIR"
echo "Board: $BOARD_FQBN"
echo "Libraries: $LIBRARIES_DIR"
echo "Output directory: $BINARIES_DIR"
echo "========================================="
echo

# Prevent literal "listing*" when no files match.
shopt -s nullglob
APP_FILES=("$APPS_DIR"/Listing*)
shopt -u nullglob

if [ ${#APP_FILES[@]} -eq 0 ]; then
    echo -e "${RED}✗ No app files found in $APPS_DIR${NC}"
    exit 1
fi

# Iterate through all listing files in apps directory
for app_file in "${APP_FILES[@]}"; do
    # Get the base name without path
    app_name=$(basename "$app_file")

    echo -e "${YELLOW}Processing: $app_name${NC}"

    # Copy the app file to mytest.ino
    cp "$app_file" "$MYTEST_INO"

    # Build the sketch
    echo "Building..."
    rm -rf "$BUILD_DIR"
    mkdir -p "$BUILD_DIR"
    if arduino-cli "${CLI_ARGS[@]}" compile --fqbn "$BOARD_FQBN" "$MYTEST_DIR" --output-dir "$BUILD_DIR" --libraries "$LIBRARIES_DIR" > "$BUILD_DIR/compile.log" 2>&1; then
        echo -e "${GREEN}✓ Build successful${NC}"

        # Find the .bin file in the build directory
        BIN_FILE="$BUILD_DIR/$(basename "$MYTEST_INO").bin"
        if [ ! -f "$BIN_FILE" ]; then
            BIN_FILE=$(find "$BUILD_DIR" -maxdepth 1 -type f -name "*.ino.bin" | head -1)
        fi
        if [ -z "$BIN_FILE" ] || [ ! -f "$BIN_FILE" ]; then
            BIN_FILE=$(find "$BUILD_DIR" -maxdepth 1 -type f -name "*.bin" ! -name "bootloader.bin" ! -name "partitions.bin" | head -1)
        fi

        if [ -n "$BIN_FILE" ] && [ -f "$BIN_FILE" ]; then
            # Copy binary to binaries directory with app name
            cp "$BIN_FILE" "$BINARIES_DIR/${app_name}.bin"
            echo -e "${GREEN}✓ Binary saved: ${app_name}.bin${NC}"
            SUCCESS_COUNT=$((SUCCESS_COUNT + 1))
        else
            echo -e "${RED}✗ Binary file not found${NC}"
            echo "  build log: $BUILD_DIR/compile.log"
            FAIL_COUNT=$((FAIL_COUNT + 1))
            FAILED_APPS+=("$app_name (binary not found)")
        fi
    else
        echo -e "${RED}✗ Build failed${NC}"
        tail -n 20 "$BUILD_DIR/compile.log"
        FAIL_COUNT=$((FAIL_COUNT + 1))
        FAILED_APPS+=("$app_name (compile error)")
    fi

    echo
done

# Summary
echo "========================================="
echo "Build Summary"
echo "========================================="
echo -e "${GREEN}Successful builds: $SUCCESS_COUNT${NC}"
echo -e "${RED}Failed builds: $FAIL_COUNT${NC}"

if [ $FAIL_COUNT -gt 0 ]; then
    echo
    echo "Failed apps:"
    for failed_app in "${FAILED_APPS[@]}"; do
        echo -e "  ${RED}✗ $failed_app${NC}"
    done
fi

echo
echo "Binaries saved in: $BINARIES_DIR"
ls -lh "$BINARIES_DIR"

echo
echo "Done!"
