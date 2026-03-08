# Converting TFLite Model to ESP32 Format

Since you have a `.tflite` file, the conversion is straightforward!

## Quick Start (3 Steps)

### Step 1: Place your TFLite file
Copy your `.tflite` file to this directory:
```bash
cp /path/to/your/model.tflite /home/odurgut/book/arduinotest/examples/mytest/
```

### Step 2: Convert to C header
Run the conversion script:
```bash
cd /home/odurgut/book/arduinotest/examples/mytest/
python3 convert_tflite_to_header.py your_model.tflite mobilenet_tflite_model.h
```

**Example:**
```bash
python3 convert_tflite_to_header.py mobilenet_segmentation.tflite mobilenet_tflite_model.h
```

### Step 3: Update the Arduino sketch
The script will tell you the exact variable name. Update line 16 in `mobilenet_esp32.ino`:

```cpp
// Change this line based on script output:
#include "mobilenet_tflite_model.h"
```

And update line 111 (in setup function):
```cpp
// Use the variable name from the script output
model = tflite::GetModel(your_model_name_tflite);
```

---

## Alternative: Using xxd (Linux/Mac)

If you prefer using command-line tools:

```bash
# Convert TFLite to header
xxd -i your_model.tflite > mobilenet_tflite_model.h

# Edit the generated file to add alignas and const
# Change the first line from:
#   unsigned char your_model_tflite[] = {
# To:
#   alignas(8) const unsigned char your_model_tflite[] = {
```

---

## Required Libraries Summary

### For ESP32 Development:

1. **embedDIP** - Your existing library ✅
   - Image capture and processing
   - Already installed

2. **TFLiteMicro** - Built into ESP32 Arduino Core ✅
   - Path: `~/.arduino15/packages/esp32/hardware/esp32/3.0.7/libraries/TFLiteMicro/`
   - No installation needed if you have ESP32 board support

3. **ESP32 Arduino Core (v3.0.7+)** ✅
   - Install via Arduino IDE:
     - `Tools > Board > Boards Manager`
     - Search "ESP32"
     - Install "esp32 by Espressif Systems"

### For Model Conversion (on your PC):

1. **Python 3** (usually pre-installed on Linux)
2. **No additional packages needed** for the conversion script

---

## Verify Your TFLite Model

Before converting, you can inspect your model:

```bash
# Install TensorFlow (optional, for inspection)
pip3 install tensorflow

# Python script to inspect model
python3 << EOF
import tensorflow as tf

interpreter = tf.lite.Interpreter(model_path='your_model.tflite')
interpreter.allocate_tensors()

# Get input details
input_details = interpreter.get_input_details()
print("Input shape:", input_details[0]['shape'])
print("Input type:", input_details[0]['dtype'])

# Get output details
output_details = interpreter.get_output_details()
print("Output shape:", output_details[0]['shape'])
print("Output type:", output_details[0]['dtype'])

# Get model size
import os
size = os.path.getsize('your_model.tflite')
print(f"Model size: {size:,} bytes ({size/1024:.2f} KB)")
EOF
```

This helps verify:
- Input dimensions match 128x128x1 (grayscale)
- Output dimensions match 128x128x2 (segmentation)
- Model size fits in ESP32 memory

---

## File Structure After Conversion

```
examples/mytest/
├── mobilenet_esp32.ino               # Main sketch
├── mobilenet_tflite_model.h          # Converted model (generated)
├── convert_tflite_to_header.py       # Conversion script
├── TFLITE_CONVERSION_GUIDE.md        # This guide
├── your_model.tflite                 # Original TFLite file
└── (STM32 files - keep for reference)
```

---

## Arduino IDE Compilation Settings

**Board Settings** (Tools menu):
- **Board**: "ESP32S3 Dev Module" (or your specific board)
- **USB CDC On Boot**: "Enabled"
- **Flash Mode**: "QIO 80MHz"
- **Flash Size**: "4MB (32Mb)" or larger
- **Partition Scheme**: "Huge APP (3MB No OTA/1MB SPIFFS)"
- **PSRAM**: "OPI PSRAM" or "QSPI PSRAM" (if available)
- **Upload Speed**: "921600"
- **Arduino Runs On**: "Core 1"
- **Events Run On**: "Core 1"

---

## Memory Requirements by ESP32 Model

| ESP32 Model | SRAM | PSRAM | Suitable? | Notes |
|-------------|------|-------|-----------|-------|
| ESP32 (original) | 520KB | Optional | ⚠️ Tight | May work with small models |
| ESP32-S2 | 320KB | Optional | ⚠️ Tight | Quantized models only |
| ESP32-S3 | 512KB | Up to 8MB | ✅ **Best** | Recommended for AI |
| ESP32-C3 | 400KB | No | ❌ No | Not enough memory |

**Recommendation**: Use **ESP32-S3 with PSRAM** for MobileNet models.

---

## Testing Your Model

1. **Upload** the sketch to ESP32
2. **Open Serial Monitor** (115200 baud)
3. You should see:
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
   ```

4. **Press button** (GPIO 15 by default)
5. Check inference time

---

## Expected Performance

For MobileNetV2 on ESP32-S3 @ 240MHz:
- **Inference time**: 500ms - 2000ms (depends on model size)
- **Memory usage**: 300KB - 600KB
- **Model size**: 500KB - 2MB

To improve performance:
- ✅ Use quantized INT8 model
- ✅ Enable PSRAM
- ✅ Overclock to 240MHz
- ✅ Use ESP32-S3 (has AI acceleration)

---

## Troubleshooting

### "Model size too large"
- **Solution**: The TFLite model might be too big
- Check model size: should be < 2MB
- Use quantized model or model pruning

### "AllocateTensors() failed"
- **Solution**: Increase `kTensorArenaSize` in line 27 of the sketch
- Try: 500000, 600000, or 800000

### "Out of memory during compilation"
- **Solution**: Model header file is very large
- Use "Huge APP" partition scheme
- Or store model in SPIFFS/SD card and load at runtime

### Model runs but gives wrong results
- **Check**: Input normalization (0-1 vs -1 to 1)
- **Check**: Input dimensions match model expectations
- **Check**: Image format (RGB vs Grayscale)

---

## Next Steps

1. ✅ Provide your `.tflite` file
2. ✅ Run conversion script
3. ✅ Update the sketch with correct variable name
4. ✅ Upload to ESP32-S3
5. ✅ Test with embedDIP camera

**Ready to proceed!** Just provide the TFLite file or let me know its path.
