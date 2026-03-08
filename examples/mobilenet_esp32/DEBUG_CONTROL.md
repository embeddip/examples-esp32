# Debug Output Control

All debug Serial output is now controlled by a single `DEBUG_ENABLED` define.

## 🔧 How to Enable/Disable Debug Output

### ✅ Debug ENABLED (Default)

**Line 22 in mobilenet_esp32.ino:**
```cpp
// Debug configuration - comment out to disable all debug output
#define DEBUG_ENABLED
```

**Output:**
```
MobileNet Segmentation with embedDIP on ESP32
Creating 128x128 images...
Input image created
Output image created
Allocating 800 KB tensor arena in PSRAM...
PSRAM available: 4194304 bytes (4.00 MB)
Free PSRAM before allocation: 4000000 bytes
Tensor arena allocated successfully at 0x3f800000
Free PSRAM after allocation: 3200000 bytes
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

Button pressed - capturing image...
Running inference...
Inference completed in 1234 ms
Segmentation result sent!
```

### ❌ Debug DISABLED (Silent Mode)

**Line 22 in mobilenet_esp32.ino:**
```cpp
// Debug configuration - comment out to disable all debug output
//#define DEBUG_ENABLED
```

**Output:**
```
(no output - only data via embedDIP serial protocol)
```

## 📊 Code Size Impact

| Mode | Flash Usage | Benefit |
|------|-------------|---------|
| Debug ON | 844,189 bytes | Full diagnostics, easier debugging |
| Debug OFF | ~830,000 bytes | Saves ~14KB flash, slightly faster |

## 🛠️ Debug Macros Used

All debug output uses these macros:

```cpp
DEBUG_PRINT(x)         // Same as Serial.print(x)
DEBUG_PRINTLN(x)       // Same as Serial.println(x)
DEBUG_PRINTF(...)      // Same as Serial.printf(...)
```

### Examples in Code:

```cpp
// With DEBUG_ENABLED - prints to serial
DEBUG_PRINTLN("Model loaded successfully");
DEBUG_PRINTF("Inference completed in %lu ms\n", inference_time);

// With DEBUG_ENABLED commented out - does nothing
DEBUG_PRINTLN("Model loaded successfully");  // No output
DEBUG_PRINTF("Inference completed in %lu ms\n", inference_time);  // No output
```

## 📌 Important Notes

### Serial.begin() Still Required
Even with debug disabled, `Serial.begin(115200)` is still called. This is intentional because:
- embedDIP may use Serial for data transmission
- Keeps code structure consistent
- Minimal overhead

If you want to completely disable Serial:
```cpp
void setup() {
  #ifdef DEBUG_ENABLED
    Serial.begin(115200);
    while (!Serial);
  #endif
  // ... rest of code
}
```

### No Debug in Production
For production deployment:
1. Comment out `#define DEBUG_ENABLED`
2. Recompile
3. Upload

Benefits:
- ✅ Smaller binary size
- ✅ Faster execution (no print overhead)
- ✅ Lower power consumption
- ✅ Cleaner serial line for data

## 🔍 When to Use Each Mode

### Use Debug ON when:
- 🐛 Developing and testing
- 🔎 Troubleshooting issues
- 📊 Monitoring performance
- 🎓 Learning how the system works

### Use Debug OFF when:
- 🚀 Production deployment
- ⚡ Maximum performance needed
- 🔋 Power optimization important
- 📡 Serial line used for data only

## 📝 Quick Reference

| What You Want | Action |
|---------------|--------|
| See all debug messages | Keep `#define DEBUG_ENABLED` |
| Silent operation | Comment: `//#define DEBUG_ENABLED` |
| Only error messages | Create `DEBUG_ERROR` macro |
| Different debug levels | Add `DEBUG_LEVEL` define |

## 🎯 Example: Add Debug Levels

If you want different debug levels (ERROR, INFO, VERBOSE):

```cpp
// Debug levels
#define DEBUG_LEVEL_ERROR   1
#define DEBUG_LEVEL_INFO    2
#define DEBUG_LEVEL_VERBOSE 3

// Set your debug level here (0 = off, 1 = errors only, 2 = info, 3 = all)
#define DEBUG_LEVEL DEBUG_LEVEL_INFO

#if DEBUG_LEVEL >= DEBUG_LEVEL_ERROR
  #define DEBUG_ERROR(...) Serial.printf("[ERROR] " __VA_ARGS__)
#else
  #define DEBUG_ERROR(...)
#endif

#if DEBUG_LEVEL >= DEBUG_LEVEL_INFO
  #define DEBUG_INFO(...) Serial.printf("[INFO] " __VA_ARGS__)
#else
  #define DEBUG_INFO(...)
#endif

#if DEBUG_LEVEL >= DEBUG_LEVEL_VERBOSE
  #define DEBUG_VERBOSE(...) Serial.printf("[VERBOSE] " __VA_ARGS__)
#else
  #define DEBUG_VERBOSE(...)
#endif
```

## ✅ Current Status

**Debug is ENABLED by default** in mobilenet_esp32.ino

To disable: Comment out line 22:
```cpp
//#define DEBUG_ENABLED
```

Then recompile and upload!

---

**Summary:** One line controls all debug output. Comment it out for production, keep it for development. 🎛️
