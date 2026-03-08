#!/usr/bin/env python3
"""
Inspect TFLite model to see all operations it uses
This helps identify which operations to add to the resolver
"""

import sys

try:
    import tensorflow as tf
except ImportError:
    print("TensorFlow not installed. Install with: pip3 install tensorflow")
    sys.exit(1)

if len(sys.argv) < 2:
    print("Usage: python3 inspect_model.py model.tflite")
    sys.exit(1)

model_path = sys.argv[1]

print(f"\n{'='*70}")
print(f"Inspecting TFLite Model: {model_path}")
print(f"{'='*70}\n")

# Load the model
interpreter = tf.lite.Interpreter(model_path=model_path)
interpreter.allocate_tensors()

# Get model details
input_details = interpreter.get_input_details()
output_details = interpreter.get_output_details()

print("📊 Model Input/Output:")
print(f"  Input shape:  {input_details[0]['shape']}")
print(f"  Input dtype:  {input_details[0]['dtype']}")
print(f"  Output shape: {output_details[0]['shape']}")
print(f"  Output dtype: {output_details[0]['dtype']}")

# Get all operations used in the model
tensor_details = interpreter.get_tensor_details()
operations = set()

# Parse operations from the model
for op_idx in range(len(interpreter._get_ops_details())):
    op = interpreter._get_ops_details()[op_idx]
    op_name = op['op_name']
    operations.add(op_name)

print(f"\n🔧 Operations Used in Model ({len(operations)} unique):")
print(f"{'─'*70}")

# Map TFLite op names to TFLite Micro resolver method names
op_mapping = {
    'CONV_2D': 'AddConv2D()',
    'DEPTHWISE_CONV_2D': 'AddDepthwiseConv2D()',
    'TRANSPOSE_CONV': 'AddTransposeConv()',
    'RESHAPE': 'AddReshape()',
    'SOFTMAX': 'AddSoftmax()',
    'MEAN': 'AddMean()',
    'FULLY_CONNECTED': 'AddFullyConnected()',
    'PAD': 'AddPad()',
    'RELU': 'AddRelu()',
    'RELU6': 'AddRelu6()',
    'ADD': 'AddAdd()',
    'MUL': 'AddMul()',
    'LOGISTIC': 'AddLogistic()',
    'QUANTIZE': 'AddQuantize()',
    'DEQUANTIZE': 'AddDequantize()',
    'RESIZE_BILINEAR': 'AddResizeBilinear()',
    'SHAPE': 'AddShape()',
    'CONCATENATION': 'AddConcatenation()',
    'MAX_POOL_2D': 'AddMaxPool2D()',
    'STRIDED_SLICE': 'AddStridedSlice()',
    'PACK': 'AddPack()',
    'AVERAGE_POOL_2D': 'AddAveragePool2D()',
    'GATHER': 'AddGather()',
    'SLICE': 'AddSlice()',
    'SPLIT': 'AddSplit()',
    'TANH': 'AddTanh()',
    'LEAKY_RELU': 'AddLeakyRelu()',
    'PRELU': 'AddPrelu()',
    'HARD_SWISH': 'AddHardSwish()',
    'BATCH_MATMUL': 'AddBatchMatMul()',
}

sorted_ops = sorted(operations)

for op in sorted_ops:
    resolver_method = op_mapping.get(op, f'Add{op.title().replace("_", "")}()')
    print(f"  • {op:25s} → resolver.{resolver_method}")

print(f"\n📝 Code for Arduino Sketch:")
print(f"{'─'*70}")
print(f"static tflite::MicroMutableOpResolver<{len(operations)}> resolver;\n")

for op in sorted_ops:
    resolver_method = op_mapping.get(op, f'Add{op.title().replace("_", "")}()')
    comment = ""
    if op in ['TRANSPOSE_CONV', 'RESIZE_BILINEAR']:
        comment = "  // Segmentation upsampling"
    elif op in ['SHAPE', 'STRIDED_SLICE', 'PACK']:
        comment = "  // Dynamic operations"
    elif op in ['CONV_2D', 'DEPTHWISE_CONV_2D']:
        comment = "  // Core convolution"

    print(f"resolver.{resolver_method};{comment}")

print(f"\n{'='*70}")
print(f"✅ Inspection complete!")
print(f"{'='*70}\n")

# File size info
import os
size_bytes = os.path.getsize(model_path)
size_kb = size_bytes / 1024
print(f"Model size: {size_bytes:,} bytes ({size_kb:.2f} KB)\n")
