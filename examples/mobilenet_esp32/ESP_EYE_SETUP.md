# ESP-EYE Configuration for MobileNet Segmentation

## Your Hardware: ESP-EYE

**ESP-EYE** is perfect for this project! It has:
- ✅ **ESP32** chip (Dual-core 240MHz)
- ✅ **4MB PSRAM** (great for AI models!)
- ✅ **OV2640 Camera** (built-in)
- ✅ **4MB Flash**
- ✅ USB interface for programming

**Specifications:**
- Chip: ESP32-DOWDQ6
- SRAM: 520KB
- PSRAM: 4MB (external)
- Flash: 4MB
- Camera: OV2640 (2MP)

---

## Arduino IDE Board Settings for ESP-EYE

### Configuration:
```
Board: "ESP32 Wrover Module"  (ESP-EYE uses Wrover)
Upload Speed: 921600
Flash Frequency: 80MHz
Flash Mode: QIO
Flash Size: 4MB (32Mb)
Partition Scheme: "Huge APP (3MB No OTA/1MB SPIFFS)"
Core Debug Level: None
PSRAM: Enabled
```

### Important PSRAM Setting:
**MUST enable PSRAM** - This is crucial for running AI models!
```
Tools > PSRAM > Enabled
```

---

## Pin Configuration for ESP-EYE

### Camera Pins (OV2640):
Already defined in embedDIP, but for reference:
```cpp
// ESP-EYE Camera Pins
#define CAM_PIN_PWDN    -1  // Power down
#define CAM_PIN_RESET   -1  // Reset
#define CAM_PIN_XCLK     4  // External clock
#define CAM_PIN_SIOD    18  // I2C SDA
#define CAM_PIN_SIOC    23  // I2C SCL

#define CAM_PIN_D7      36
#define CAM_PIN_D6      37
#define CAM_PIN_D5      38
#define CAM_PIN_D4      39
#define CAM_PIN_D3      35
#define CAM_PIN_D2      14
#define CAM_PIN_D1      13
#define CAM_PIN_D0      34
#define CAM_PIN_VSYNC    5
#define CAM_PIN_HREF    27
#define CAM_PIN_PCLK    25
```

### Button Pin:
```cpp
// ESP-EYE Boot Button
#define PIN_BUTTON 0  // GPIO0 (Boot button)
```

---

## Updated Sketch for ESP-EYE

The button pin needs to be changed from GPIO15 to GPIO0. I'll update the sketch:

```cpp
#ifndef PIN_BUTTON
#define PIN_BUTTON 0  // ESP-EYE Boot button (GPIO0)
#endif
```

---

## Memory Usage for Your Model

Your model is **115KB** - excellent size!

### Memory Breakdown:
```
Model size: 115 KB (in flash)
Tensor arena: ~400 KB (in PSRAM)
embedDIP buffers: ~100 KB
Total runtime RAM: ~500 KB

ESP-EYE Available:
- SRAM: 520 KB
- PSRAM: 4 MB ✅ More than enough!
```

**Result**: Your model will fit comfortably! 🎉

---

## Compilation Fix

Your sketch is now in the correct location:
```
/home/odurgut/book/arduinotest/examples/mobilenet_esp32/
├── mobilenet_esp32.ino
└── mobilenet_tflite_model.h
```

The original `mytest` directory keeps your STM32 version separate.

---

## Expected Performance on ESP-EYE

### Inference Time Estimates:
- **Without optimization**: ~1500-2500ms per frame
- **With PSRAM**: ~1000-1500ms per frame
- **Quantized model**: ~500-1000ms per frame

For comparison:
- STM32 (your current): ~200-500ms
- ESP32-S3 (newer): ~100-300ms
- ESP-EYE (ESP32 Wrover): ~1000ms

**Note**: ESP-EYE is slower than STM32 for AI but still usable!

---

## How to Upload and Test

### Step 1: Connect ESP-EYE
1. Connect ESP-EYE via USB
2. Check port: `ls /dev/ttyUSB*` or `ls /dev/ttyACM*`
3. Give permissions if needed:
   ```bash
   sudo usermod -a -G dialout $USER
   # Then logout and login
   ```

### Step 2: Compile
Using Arduino CLI:
```bash
cd /home/odurgut/book/arduinotest
arduino-cli compile --fqbn esp32:esp32:esp32wrover examples/mobilenet_esp32/
```

Or use VSCode Arduino extension:
- Open `mobilenet_esp32.ino`
- Press F1 → "Arduino: Verify"

### Step 3: Upload
```bash
arduino-cli upload -p /dev/ttyUSB0 --fqbn esp32:esp32:esp32wrover examples/mobilenet_esp32/
```

Or in VSCode:
- Press F1 → "Arduino: Upload"

### Step 4: Monitor
```bash
arduino-cli monitor -p /dev/ttyUSB0 -c baudrate=115200
```

Or in VSCode: Serial Monitor

---

## Expected Serial Output

```
MobileNet Segmentation with embedDIP on ESP32
Loading TensorFlow Lite model...
Model loaded successfully

Model Input Info:
  Dimensions: 4
  Shape: [1, 128, 128, 1]
  Type: 1

Model Output Info:
  Dimensions: 4
  Shape: [1, 128, 128, 2]
  Type: 1

Setup complete! Press button to capture and segment image.

[Press Boot button on ESP-EYE]

Button pressed - capturing image...
Running inference...
Inference completed in 1234 ms
Segmentation result sent!
```

---

## Troubleshooting ESP-EYE

### "No serial data" or "Camera init failed"
- **Check**: PSRAM is enabled in Tools menu
- **Try**: Press Reset button after upload

### "AllocateTensors() failed"
- **Check**: PSRAM enabled
- **Increase**: `kTensorArenaSize` to 500000 or 600000

### "Upload failed" or "Connecting..."
- **Solution**: Hold Boot button while connecting
- **Check**: USB cable (must be data cable, not charge-only)

### Slow inference (>3 seconds)
- **Normal** for ESP-EYE with this model
- **Optimize**: Use quantized INT8 model for 2x speedup
- **Alternative**: Use ESP32-S3 for 5-10x speedup

### Camera not working with embedDIP
- **Check**: embedDIP camera configuration matches ESP-EYE pins
- **Test**: Use simple camera example first

---

## Optimizing for ESP-EYE

### 1. Enable PSRAM Arena
Update the sketch to use PSRAM:
```cpp
// Use PSRAM for tensor arena (line ~32)
constexpr int kTensorArenaSize = 400 * 1024;
uint8_t *tensor_arena = (uint8_t*)ps_malloc(kTensorArenaSize);
```

### 2. Reduce Image Size
If 128x128 is too slow, try 96x96:
```cpp
#define INPUT_WIDTH 96
#define INPUT_HEIGHT 96
```
(Requires retraining model with new input size)

### 3. Use Quantized Model
Convert to INT8:
```python
converter.target_spec.supported_ops = [tf.lite.OpsSet.TFLITE_BUILTINS_INT8]
```
This gives ~2x speedup!

---

## Comparison: ESP-EYE vs STM32

| Feature | STM32 + STM32Cube.AI | ESP-EYE + TFLite |
|---------|----------------------|------------------|
| **Inference Speed** | 200-500ms | 1000-1500ms |
| **AI Acceleration** | ✅ Yes (proprietary) | ❌ No (SW only) |
| **Memory** | 256KB-1MB | 520KB + 4MB PSRAM |
| **Camera** | External | ✅ Built-in (OV2640) |
| **WiFi/BT** | External | ✅ Built-in |
| **Development** | CubeIDE/Keil | ✅ Arduino (easier) |
| **Model Format** | STM32Cube.AI | TensorFlow Lite |
| **Cost** | $10-30 | ~$15 |

**ESP-EYE Advantages:**
- ✅ Built-in camera
- ✅ WiFi/Bluetooth
- ✅ Easier development (Arduino)
- ✅ More RAM (with PSRAM)

**STM32 Advantages:**
- ✅ Faster inference (with AI acceleration)
- ✅ Lower power consumption
- ✅ Better real-time performance

---

## Next Steps

1. ✅ **Compile** the sketch for ESP-EYE
2. ✅ **Upload** to your board
3. ✅ **Test** with Serial Monitor
4. ✅ **Verify** inference works with embedDIP
5. 📊 **Measure** actual inference time
6. 🚀 **Optimize** if needed (quantization, smaller input)

---

## Need Help?

Check these if you have issues:
- ESP-EYE schematic: Available on Espressif website
- embedDIP docs: Check your library documentation
- TFLite Micro: Built into ESP32 Arduino core
- This project: Already configured! 🎉

**Your setup is ready to compile and test!**
