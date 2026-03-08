# Performance Optimization Guide

## 🔍 Current Setup: ESP-EYE (ESP32)

Your ESP-EYE uses **ESP32 (not S3)**, so we need ESP32-compatible optimizations.

## ⚡ Performance Comparison

| Hardware | Current Speed | With Optimization | Speedup |
|----------|--------------|-------------------|---------|
| ESP-EYE (ESP32) | ~1200-1500ms | ~400-800ms | 2-3x faster |
| ESP32-S3 | ~800-1200ms | ~100-300ms | 10x faster |

---

## 🚀 Option 1: Use Espressif's ESP-NN (Recommended for ESP32)

Espressif provides optimized kernels for **all ESP32 variants** including your ESP-EYE.

### Installation:

**Method A: Use esp-tflite-micro (ESP-IDF - Best Performance)**

This requires ESP-IDF (not Arduino), but gives best results:

```bash
# Install ESP-IDF
git clone --recursive https://github.com/espressif/esp-idf.git
cd esp-idf
./install.sh

# Create new project
. ./export.sh
idf.py create-project mobilenet_esp32

# Add esp-tflite-micro dependency
cd mobilenet_esp32
idf.py add-dependency "esp-tflite-micro"
```

**Performance gain**: 2-3x faster than current Arduino implementation

**Downside**: Requires converting from Arduino to ESP-IDF framework

---

**Method B: Manual ESP-NN Integration in Arduino (Experimental)**

1. Download ESP-NN library:
```bash
cd ~/Arduino/libraries
git clone https://github.com/espressif/esp-nn.git
```

2. Modify your sketch to use ESP-NN kernels (requires expertise)

**Complexity**: High - requires deep TFLite knowledge
**Gain**: 1.5-2x faster

---

## 🎯 Option 2: Model Optimization (Easiest, Works Now!)

### A. Quantize Model (INT8 instead of FLOAT32)

**Current**: Your model uses FLOAT32 (4 bytes per weight)
**Optimized**: INT8 quantization (1 byte per weight)

**Benefits**:
- 4x smaller model size (115KB → ~30KB)
- 2-3x faster inference
- Same accuracy (within 1-2%)

**How to convert:**

```python
import tensorflow as tf
import numpy as np

# Load your model
model = tf.keras.models.load_model('mobilenetv2_segmentation.h5')

# Create representative dataset for quantization
def representative_dataset():
    for _ in range(100):
        # Generate sample data matching your model input
        data = np.random.rand(1, 128, 128, 1).astype(np.float32)
        yield [data]

# Convert with INT8 quantization
converter = tf.lite.TFLiteConverter.from_keras_model(model)
converter.optimizations = [tf.lite.Optimize.DEFAULT]
converter.representative_dataset = representative_dataset
converter.target_spec.supported_ops = [tf.lite.OpsSet.TFLITE_BUILTINS_INT8]
converter.inference_input_type = tf.uint8
converter.inference_output_type = tf.uint8

# Convert and save
tflite_quant_model = converter.convert()
with open('mobilenet_int8.tflite', 'wb') as f:
    f.write(tflite_quant_model)

print(f"Quantized model size: {len(tflite_quant_model)} bytes")
```

Then convert the quantized model:
```bash
python3 convert_tflite_to_header.py mobilenet_int8.tflite mobilenet_int8_model.h
```

**Expected speed**: ~400-600ms (vs current 1200-1500ms)

---

### B. Reduce Input Size

**Current**: 128x128 = 16,384 pixels
**Optimized**: 96x96 = 9,216 pixels (44% fewer pixels)

Change in sketch:
```cpp
#define INPUT_WIDTH 96
#define INPUT_HEIGHT 96
```

**Note**: Requires retraining model with 96x96 input

**Expected speed**: ~700-900ms

---

### C. Use Lower Precision (FLOAT16)

Some ESP32-S3 boards support FLOAT16, but regular ESP32 doesn't benefit much.

**Skip this for ESP32**

---

## 🔧 Option 3: Code-Level Optimizations

### A. Enable Compiler Optimizations

In Arduino IDE:
```
Tools > Core Debug Level > None
Tools > Optimization > Faster (-O2)
```

**Gain**: 5-10% faster

### B. Overclock ESP32 (Use with Caution)

ESP-EYE can run at 240MHz (default is 160MHz):

```cpp
void setup() {
  setCpuFrequencyMhz(240);  // Max speed
  // ... rest of setup
}
```

**Gain**: ~20-30% faster
**Warning**: Higher power consumption, more heat

### C. Reduce Stack Size for More PSRAM

Currently:
```cpp
SET_LOOP_TASK_STACK_SIZE(16 * 1024 * 2);  // 32KB
```

Can reduce to:
```cpp
SET_LOOP_TASK_STACK_SIZE(16 * 1024);  // 16KB
```

**Gain**: Frees memory, minimal speed improvement

---

## 💡 Option 4: Hardware Upgrade (Most Effective)

### Upgrade to ESP32-S3 Board

If you can get an ESP32-S3 board (not just software change):

| Board | Price | Speed Gain | Notes |
|-------|-------|------------|-------|
| **ESP32-S3-DevKitC** | ~$10 | 5-10x faster | Official Espressif board |
| **ESP32-S3-EYE** | ~$20 | 5-10x faster | Has camera like ESP-EYE |
| **Seeed XIAO ESP32-S3** | ~$7 | 5-10x faster | Tiny, with camera |

**ESP32-S3 Benefits**:
- AI acceleration (vector instructions)
- Works with j-siderius library
- 8MB PSRAM standard
- ~100-300ms inference (vs 1200ms now)

---

## 📊 Recommended Optimization Path

### For ESP-EYE (ESP32) - Immediate:

1. **Quantize model to INT8** ✅ Best ROI
   - Expected: 1200ms → 400-600ms (2-3x faster)
   - Effort: 1 hour (run Python script)

2. **Overclock to 240MHz** ✅ Easy
   - Expected: Additional 20-30% faster
   - Effort: 1 line of code

3. **Enable -O2 optimization** ✅ Free
   - Expected: 5-10% faster
   - Effort: Change Arduino settings

**Combined result**: 1200ms → ~350-450ms (3x faster!)

### Long-term:

4. **Upgrade to ESP32-S3** 🎯 Best performance
   - Expected: ~100-300ms (10x faster than now)
   - Cost: $10-20
   - Can use j-siderius optimized library

---

## 🛠️ Quick Implementation: Quantization + Overclocking

### Step 1: Overclock (Add to setup() now)

```cpp
void setup() {
  setCpuFrequencyMhz(240);  // Add this line first
  Serial.begin(115200);
  // ... rest of setup
}
```

### Step 2: Quantize Model (Run on PC)

Save this as `quantize_model.py`:

```python
import tensorflow as tf
import numpy as np

# Load model
model = tf.keras.models.load_model('mobilenetv2_segmentation.h5')

def representative_dataset():
    for _ in range(100):
        yield [np.random.rand(1, 128, 128, 1).astype(np.float32)]

# Quantize
converter = tf.lite.TFLiteConverter.from_keras_model(model)
converter.optimizations = [tf.lite.Optimize.DEFAULT]
converter.representative_dataset = representative_dataset
converter.target_spec.supported_ops = [tf.lite.OpsSet.TFLITE_BUILTINS_INT8]
converter.inference_input_type = tf.uint8
converter.inference_output_type = tf.uint8

# Save
tflite_quant = converter.convert()
with open('mobilenet_int8.tflite', 'wb') as f:
    f.write(tflite_quant)
print(f"Quantized: {len(tflite_quant):,} bytes")
```

Run:
```bash
python3 quantize_model.py
python3 convert_tflite_to_header.py mobilenet_int8.tflite mobilenet_int8_model.h
```

### Step 3: Update Arduino Sketch

Replace line 34:
```cpp
#include "mobilenet_tflite_model.h"
```
With:
```cpp
#include "mobilenet_int8_model.h"
```

---

## 📈 Expected Results Summary

| Optimization | Current Speed | After | Effort |
|--------------|---------------|-------|--------|
| None | 1200-1500ms | - | - |
| Overclock 240MHz | 1200ms | ~900ms | 5 min |
| INT8 Quantization | 1200ms | ~400ms | 1 hour |
| Both | 1200ms | ~350ms | 1 hour |
| + ESP32-S3 upgrade | 1200ms | ~100ms | $15 + 1 day |

---

## ❓ FAQ

**Q: Can I use the j-siderius library on ESP32?**
A: No, it's ESP32-S3 only. For ESP32, use quantization + overclocking.

**Q: Will quantization reduce accuracy?**
A: Typically <2% accuracy loss, often negligible for segmentation.

**Q: Is overclocking safe?**
A: Yes at 240MHz (ESP32 rated speed), but increases power/heat slightly.

**Q: Should I upgrade to ESP32-S3?**
A: If speed is critical and you do AI regularly, yes. 10x faster is worth $15.

---

## 🎯 My Recommendation

**Immediate (This Week)**:
1. Add overclocking (1 line of code) → 20-30% faster
2. Quantize your model to INT8 → 2-3x faster
3. **Total gain**: ~3x faster (1200ms → 400ms)

**Future (If Serious About AI)**:
- Buy ESP32-S3-EYE (~$20) → 10x faster total
- Use j-siderius optimized library
- Get ~100ms inference time

Let me know which approach you want to try first! 🚀
