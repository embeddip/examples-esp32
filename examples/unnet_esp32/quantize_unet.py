#!/usr/bin/env python3
"""
Quantize UNet model from float32 to INT8 for ESP32
Reduces memory usage by ~4x and speeds up inference by 2-4x
"""
import tensorflow as tf
import numpy as np

def representative_dataset():
    """Generate representative data for quantization calibration"""
    print("Generating representative dataset (100 samples)...")
    for i in range(100):
        # Generate random grayscale 128x128 images
        data = np.random.rand(1, 128, 128, 1).astype(np.float32)
        yield [data]
        if (i + 1) % 20 == 0:
            print(f"  Generated {i + 1}/100 samples")

# Load the existing float32 TFLite model
print("\nLoading unet.tflite...")
with open('unet.tflite', 'rb') as f:
    tflite_model = f.read()

print(f"Original model size: {len(tflite_model)} bytes ({len(tflite_model)/1024:.1f} KB)")

# Create converter from the model content
converter = tf.lite.TFLiteConverter.from_tflite_model(tflite_model)

# Enable full INT8 quantization
print("\nConfiguring INT8 quantization...")
converter.optimizations = [tf.lite.Optimize.DEFAULT]
converter.representative_dataset = representative_dataset

# Force INT8 for all operations (not just weights)
converter.target_spec.supported_ops = [tf.lite.OpsSet.TFLITE_BUILTINS_INT8]

# Keep input/output as float32 for easier use (convert internally)
# If you want full INT8 including inputs, uncomment these:
# converter.inference_input_type = tf.int8
# converter.inference_output_type = tf.int8

print("\nQuantizing model (this may take 1-2 minutes)...")
tflite_model_quant = converter.convert()

# Save quantized model
output_file = 'unet_int8.tflite'
with open(output_file, 'wb') as f:
    f.write(tflite_model_quant)

print(f"\n✅ Quantization complete!")
print(f"  Original: {len(tflite_model)} bytes ({len(tflite_model)/1024:.1f} KB)")
print(f"  Quantized: {len(tflite_model_quant)} bytes ({len(tflite_model_quant)/1024:.1f} KB)")
print(f"  Reduction: {len(tflite_model) / len(tflite_model_quant):.2f}x")
print(f"  Saved to: {output_file}")

print("\nNext steps:")
print("1. Convert to header:")
print("   xxd -i unet_int8.tflite > unet_int8_model.h")
print("2. Update .ino file to include 'unet_int8_model.h'")
print("3. Reduce kTensorArenaSize to ~400KB (was 1.3MB)")
