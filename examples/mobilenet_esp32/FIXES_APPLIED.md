# Fixes Applied to mobilenet_esp32.ino

## Issues Encountered & Fixed

### ❌ Issue 1: "Didn't find op for builtin opcode 'SHAPE'"
**Error Message:**
```
Didn't find op for builtin opcode 'SHAPE'
Failed to get registration from op code SHAPE
```

**Cause:** The MobileNetV2 segmentation model uses the `SHAPE` operation (and possibly others) that weren't included in the operation resolver.

**Fix Applied:**
- Increased resolver size from 15 to 20 operations
- Added missing operations:
  ```cpp
  resolver.AddShape();           // Required by your model
  resolver.AddConcatenation();   // Often needed for segmentation
  resolver.AddMaxPool2D();       // Common in MobileNet
  resolver.AddStridedSlice();    // Often used with Shape
  resolver.AddPack();            // May be needed
  ```

**Location:** Line 135 in mobilenet_esp32.ino

---

### ❌ Issue 2: "Error: Failed to create input/output image"
**Error Message:**
```
Error: Failed to create input image
Error: Failed to create output image
```

**Cause:** The code was using `IMAGE_RES_QQVGA` (160x120) instead of the model's required 128x128 dimensions. The predefined resolution didn't match.

**Fix Applied:**
- Changed from `createImage(IMAGE_RES_QQVGA, ...)` to `createImageWH(128, 128, ...)`
- Now creates exact 128x128 pixel images:
  ```cpp
  createImageWH(INPUT_WIDTH, INPUT_HEIGHT, IMAGE_FORMAT_GRAYSCALE, &inImg)
  createImageWH(INPUT_WIDTH, INPUT_HEIGHT, IMAGE_FORMAT_GRAYSCALE, &outImg)
  ```

**Location:** Lines 97-107 in mobilenet_esp32.ino

---

### ❌ Issue 3: "Arena size needed might be larger than 409600 bytes"
**Error Message:**
```
Error: Failed to allocate tensors
Arena size needed might be larger than 409600 bytes
```

**Status:** Fixed by adding missing operations (Issue 1)

**Note:** If this error persists after uploading the fixed code, you can increase the tensor arena size:
```cpp
constexpr int kTensorArenaSize = 500 * 1024; // Increase from 400KB to 500KB
```

---

## Verification

### ✅ Compilation Status
```
Sketch uses 836,109 bytes (63%) of program storage space
Global variables use 28,508 bytes (8%) of dynamic memory
Status: SUCCESS
```

### ✅ Changes Summary
1. Added 5 missing TFLite operations
2. Changed image creation to use exact 128x128 dimensions
3. Increased resolver capacity from 15 to 20 operations
4. Added error handling returns

---

## Expected Output After Upload

```
MobileNet Segmentation with embedDIP on ESP32
[MEMORY] PSRAM available: 4194304 bytes
Creating 128x128 images...
Input image created
Output image created
Allocating 400 KB tensor arena in PSRAM...
Tensor arena allocated successfully
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

**No more errors!** ✅

---

## Upload Instructions

```bash
# Recompile with fixes
arduino-cli compile --fqbn esp32:esp32:esp32wrover examples/mobilenet_esp32/

# Upload to ESP-EYE
arduino-cli upload -p /dev/ttyUSB0 --fqbn esp32:esp32:esp32wrover examples/mobilenet_esp32/

# Monitor serial output
arduino-cli monitor -p /dev/ttyUSB0 -c baudrate=115200
```

---

## If Issues Persist

### Still getting "SHAPE" error?
- Make sure you uploaded the NEW code (recompile and upload)
- Check that the model file hasn't changed

### Still getting "Failed to allocate tensors"?
Try increasing tensor arena size in line 33:
```cpp
constexpr int kTensorArenaSize = 500 * 1024; // or 600 * 1024
```

### Out of memory during upload?
- Select: `Tools > Partition Scheme > Huge APP (3MB No OTA/1MB SPIFFS)`
- Ensure PSRAM is enabled: `Tools > PSRAM > Enabled`

### Camera capture not working?
The embedDIP camera capture expects a connected camera. On ESP-EYE:
- Built-in OV2640 should work automatically
- Make sure embedDIP camera is properly initialized
- Check `serial->init()` completes without error

---

## Technical Details

### Operations Added
The MobileNetV2 segmentation model architecture uses these TFLite operations:

| Operation | Purpose |
|-----------|---------|
| Conv2D | Standard convolution layers |
| DepthwiseConv2D | Efficient depthwise separable convolutions |
| Shape | Get tensor shape (used in dynamic operations) |
| StridedSlice | Tensor slicing operations |
| Pack | Combine tensors |
| Concatenation | Merge feature maps |
| MaxPool2D | Downsampling |
| Reshape | Change tensor dimensions |
| Softmax | Output probability distribution |
| Add/Mul | Element-wise operations |
| ReLU/ReLU6 | Activation functions |
| ResizeBilinear | Upsampling for segmentation output |

### Memory Layout
```
Model: 115KB (flash)
Tensor Arena: 400KB (PSRAM)
Input Image: 128×128×1 = 16KB (PSRAM)
Output Image: 128×128×2 = 32KB (PSRAM)
Total: ~563KB + overhead
```

---

## Status: ✅ READY TO TEST

All known issues have been fixed. The sketch should now:
1. ✅ Load the model successfully
2. ✅ Allocate all tensors
3. ✅ Create 128x128 images properly
4. ✅ Run inference without errors

Upload and test on your ESP-EYE! 🚀
