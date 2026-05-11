# Embedded Digital Image Processing with Microcontrollers: Applications on Arduino Boards

Example ESP32 sketches and support files from the book.

## Requirements

- Arduino CLI
- ESP32 Arduino core
- A supported ESP32 board
- USB serial access to the board, usually `/dev/ttyUSB0` on Linux

Install or update the board indexes:

```bash
arduino-cli --config-file arduino-cli.yaml core update-index
```

Install the default ESP32 core:

```bash
arduino-cli --config-file arduino-cli.yaml core install esp32:esp32
```

## Build

Compile the current test sketch:

```bash
arduino-cli compile --config-file arduino-cli.yaml --fqbn "$(tr -d '\r\n' < boards.fqbn)" examples/mytest/
```

Build every listing in `apps/`:

```bash
./build_all_apps.sh
```

The generated `.bin` files are written to `binaries/`.

## Upload

Upload the current test sketch:

```bash
arduino-cli upload --config-file arduino-cli.yaml --fqbn "$(tr -d '\r\n' < boards.fqbn)" --port /dev/ttyUSB0 examples/mytest/
```

Change `/dev/ttyUSB0` if your board appears on a different serial port.

## Board Configuration

The default board target is stored in `boards.fqbn`. To use another board, edit that file or set `BOARD_FQBN` when running the build script:

```bash
BOARD_FQBN="esp32:esp32:esp32:PSRAM=enabled" ./build_all_apps.sh
```

Some listings require PSRAM, so use a board and FQBN configuration that enables it.
