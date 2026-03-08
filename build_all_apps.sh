#!/bin/bash

# Configuration
APPS_DIR="/home/odurgut/book/arduinotest/apps"
MYTEST_DIR="/home/odurgut/book/arduinotest/examples/mytest"
MYTEST_INO="$MYTEST_DIR/mytest.ino"
BINARIES_DIR="/home/odurgut/book/arduinotest/binaries"
BOARD_FQBN="esp32:esp32:esp32wroverkit"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Create binaries directory if it doesn't exist
mkdir -p "$BINARIES_DIR"

# Keep track of successes and failures
SUCCESS_COUNT=0
FAIL_COUNT=0
FAILED_APPS=()

echo "========================================="
echo "Building all apps from $APPS_DIR"
echo "Board: $BOARD_FQBN"
echo "Output directory: $BINARIES_DIR"
echo "========================================="
echo

# Iterate through all listing files in apps directory
for app_file in "$APPS_DIR"/listing*; do
    # Get the base name without path
    app_name=$(basename "$app_file")

    echo -e "${YELLOW}Processing: $app_name${NC}"

    # Copy the app file to mytest.ino
    cp "$app_file" "$MYTEST_INO"

    # Build the sketch
    echo "Building..."
    if arduino-cli compile --fqbn "$BOARD_FQBN" "$MYTEST_DIR" --output-dir "$MYTEST_DIR/build" --libraries "/home/odurgut/book/arduinotest/libraries" > /dev/null 2>&1; then
        echo -e "${GREEN}✓ Build successful${NC}"

        # Find the .bin file in the build directory
        BIN_FILE=$(find "$MYTEST_DIR/build" -name "*.bin" | head -1)

        if [ -n "$BIN_FILE" ]; then
            # Copy binary to binaries directory with app name
            cp "$BIN_FILE" "$BINARIES_DIR/${app_name}.bin"
            echo -e "${GREEN}✓ Binary saved: ${app_name}.bin${NC}"
            SUCCESS_COUNT=$((SUCCESS_COUNT + 1))
        else
            echo -e "${RED}✗ Binary file not found${NC}"
            FAIL_COUNT=$((FAIL_COUNT + 1))
            FAILED_APPS+=("$app_name (binary not found)")
        fi
    else
        echo -e "${RED}✗ Build failed${NC}"
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

# Clean up build directory
rm -rf "$MYTEST_DIR/build"

echo
echo "Done!"
