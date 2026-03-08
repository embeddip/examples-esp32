#!/usr/bin/env python3
"""
Convert TensorFlow Lite model (.tflite) to Arduino C header file

Usage: python convert_tflite_to_header.py input_model.tflite output_model.h
"""

import sys
import os

def convert_tflite_to_header(tflite_path, header_path):
    """Convert TFLite binary to C header file"""

    # Read the TFLite model
    with open(tflite_path, 'rb') as f:
        model_data = f.read()

    model_size = len(model_data)
    print(f"Model size: {model_size:,} bytes ({model_size/1024:.2f} KB)")

    # Extract base name for variable naming
    base_name = os.path.splitext(os.path.basename(tflite_path))[0]
    variable_name = base_name.replace('-', '_').replace('.', '_').lower()

    # Generate C header file
    with open(header_path, 'w') as f:
        # Header guard
        guard_name = variable_name.upper() + '_H'
        f.write(f'#ifndef {guard_name}\n')
        f.write(f'#define {guard_name}\n\n')

        # Include statement
        f.write('#include <stdint.h>\n\n')

        # Array declaration with alignment
        f.write(f'// TensorFlow Lite model converted from {os.path.basename(tflite_path)}\n')
        f.write(f'// Model size: {model_size:,} bytes\n')
        f.write(f'alignas(8) const unsigned char {variable_name}_tflite[] = {{\n')

        # Write data in rows of 12 bytes
        bytes_per_row = 12
        for i in range(0, model_size, bytes_per_row):
            chunk = model_data[i:i + bytes_per_row]
            hex_values = ', '.join([f'0x{b:02x}' for b in chunk])
            f.write(f'  {hex_values},\n')

        f.write('};\n\n')

        # Array length
        f.write(f'const unsigned int {variable_name}_tflite_len = {model_size};\n\n')

        # Close header guard
        f.write(f'#endif  // {guard_name}\n')

    print(f'Successfully created: {header_path}')
    print(f'Variable name: {variable_name}_tflite')
    print(f'\nTo use in your Arduino sketch:')
    print(f'  #include "{os.path.basename(header_path)}"')
    print(f'  model = tflite::GetModel({variable_name}_tflite);')

if __name__ == '__main__':
    if len(sys.argv) < 2:
        print('Usage: python convert_tflite_to_header.py input_model.tflite [output_model.h]')
        print('\nExample:')
        print('  python convert_tflite_to_header.py mobilenet.tflite mobilenet_tflite_model.h')
        sys.exit(1)

    tflite_file = sys.argv[1]

    if not os.path.exists(tflite_file):
        print(f'Error: File not found: {tflite_file}')
        sys.exit(1)

    if len(sys.argv) >= 3:
        header_file = sys.argv[2]
    else:
        # Auto-generate header filename
        base = os.path.splitext(tflite_file)[0]
        header_file = base + '_model.h'

    try:
        convert_tflite_to_header(tflite_file, header_file)
    except Exception as e:
        print(f'Error: {e}')
        sys.exit(1)
