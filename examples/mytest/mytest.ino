/**
 * MobileNet Segmentation on ESP32 with embedDIP
 *
 * This example shows how to run MobileNetV2 segmentation model
 * on ESP32 using TensorFlow Lite Micro and embedDIP for image capture
 *
 * Hardware: ESP32 with camera module
 * Libraries: TFLiteMicro (built-in), embedDIP
 */

#include "embedDIP.h"
#include <Arduino.h>
#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/system_setup.h"
#include "tensorflow/lite/schema/schema_generated.h"

// Include your converted TFLite model
#include "mobilenet_tflite_model.h"

#ifndef PIN_BUTTON
#define PIN_BUTTON 15
#endif

// Model input/output dimensions (matching STM32 version)
#define INPUT_WIDTH 128
#define INPUT_HEIGHT 128
#define INPUT_CHANNELS 1  // Grayscale
#define OUTPUT_CHANNELS 2 // Segmentation classes

// TensorFlow Lite settings
constexpr int kTensorArenaSize = 400 * 1024; // 400KB for MobileNetV2
alignas(16) uint8_t tensor_arena[kTensorArenaSize];

// Global variables
SET_LOOP_TASK_STACK_SIZE(16 * 1024 * 2);

serial_t *serial = &esp32_uart;
Image *inImg = nullptr;
Image *outImg = nullptr;

// TFLite variables
const tflite::Model* model = nullptr;
tflite::MicroInterpreter* interpreter = nullptr;
TfLiteTensor* input_tensor = nullptr;
TfLiteTensor* output_tensor = nullptr;

// Function to convert embedDIP image to model input format
bool prepareModelInput(Image* img, float* input_buffer) {
  if (img->width != INPUT_WIDTH || img->height != INPUT_HEIGHT) {
    Serial.println("Error: Image dimensions don't match model input");
    return false;
  }

  // Normalize grayscale image data to float [0.0, 1.0] or [-1.0, 1.0]
  // depending on your model's training preprocessing
  for (int i = 0; i < INPUT_WIDTH * INPUT_HEIGHT; i++) {
    // Assuming grayscale 8-bit data
    uint8_t pixel = img->data[i];
    input_buffer[i] = (float)pixel / 255.0f;  // Normalize to [0, 1]
    // OR use: input_buffer[i] = ((float)pixel / 127.5f) - 1.0f;  // Normalize to [-1, 1]
  }

  return true;
}

// Function to process model output to embedDIP image
void processModelOutput(float* output_buffer, Image* outImg) {
  // Output is 128x128x2 (two channels for segmentation)
  // Convert to grayscale by taking argmax or channel 1

  for (int i = 0; i < INPUT_WIDTH * INPUT_HEIGHT; i++) {
    // Take argmax between two channels
    float class0 = output_buffer[i * 2];
    float class1 = output_buffer[i * 2 + 1];

    // Set pixel to 255 if class1 is higher, 0 otherwise
    outImg->data[i] = (class1 > class0) ? 255 : 0;
  }
}

void setup() {
  Serial.begin(115200);
  while (!Serial);

  Serial.println("MobileNet Segmentation with embedDIP on ESP32");

  // Initialize embedDIP
  serial->init();

  // Create images (128x128 to match model input)
  if (!createImage(IMAGE_RES_QQVGA, IMAGE_FORMAT_GRAYSCALE, &inImg)) {
    Serial.println("Error: Failed to create input image");
    // Note: IMAGE_RES_QQVGA is 160x120, you may need to resize or use custom dimensions
    // For exact 128x128, you might need to crop or implement custom createImage
  }

  if (!createImage(IMAGE_RES_QQVGA, IMAGE_FORMAT_GRAYSCALE, &outImg)) {
    Serial.println("Error: Failed to create output image");
  }

  pinMode(PIN_BUTTON, INPUT_PULLUP);

  // Initialize TensorFlow Lite
  Serial.println("Loading TensorFlow Lite model...");

  // Load the model
  model = tflite::GetModel(mobilenetv2_segmentation_tflite);
  if (model->version() != TFLITE_SCHEMA_VERSION) {
    Serial.printf("Model version mismatch: %d vs %d\n",
                  model->version(), TFLITE_SCHEMA_VERSION);
    return;
  }
  Serial.println("Model loaded successfully");

  // Set up the operations resolver
  // Add only the operations used by your model to save memory
  static tflite::MicroMutableOpResolver<15> resolver;

  // Common operations for MobileNetV2
  resolver.AddConv2D();
  resolver.AddDepthwiseConv2D();
  resolver.AddReshape();
  resolver.AddSoftmax();
  resolver.AddMean();
  resolver.AddFullyConnected();
  resolver.AddPad();
  resolver.AddRelu();
  resolver.AddRelu6();
  resolver.AddAdd();
  resolver.AddMul();
  resolver.AddLogistic();
  resolver.AddQuantize();
  resolver.AddDequantize();
  resolver.AddResizeBilinear();

  // Build the interpreter
  static tflite::MicroInterpreter static_interpreter(
    model, resolver, tensor_arena, kTensorArenaSize);
  interpreter = &static_interpreter;

  // Allocate memory for tensors
  TfLiteStatus allocate_status = interpreter->AllocateTensors();
  if (allocate_status != kTfLiteOk) {
    Serial.println("Error: Failed to allocate tensors");
    Serial.printf("Arena size needed might be larger than %d bytes\n", kTensorArenaSize);
    return;
  }

  // Get input and output tensors
  input_tensor = interpreter->input(0);
  output_tensor = interpreter->output(0);

  // Print tensor info for debugging
  Serial.println("\nModel Input Info:");
  Serial.printf("  Dimensions: %d\n", input_tensor->dims->size);
  Serial.printf("  Shape: [%d, %d, %d, %d]\n",
                input_tensor->dims->data[0],
                input_tensor->dims->data[1],
                input_tensor->dims->data[2],
                input_tensor->dims->data[3]);
  Serial.printf("  Type: %d\n", input_tensor->type);

  Serial.println("\nModel Output Info:");
  Serial.printf("  Dimensions: %d\n", output_tensor->dims->size);
  Serial.printf("  Shape: [%d, %d, %d, %d]\n",
                output_tensor->dims->data[0],
                output_tensor->dims->data[1],
                output_tensor->dims->data[2],
                output_tensor->dims->data[3]);
  Serial.printf("  Type: %d\n", output_tensor->type);

  Serial.println("\nSetup complete! Press button to capture and segment image.");
}

void loop() {
  if (digitalRead(PIN_BUTTON) == LOW) {
    Serial.println("Button pressed - capturing image...");

    // Capture image using embedDIP
    serial->capture(inImg);

    // Prepare input data for the model
    float* input_data = input_tensor->data.f;
    if (!prepareModelInput(inImg, input_data)) {
      Serial.println("Error preparing model input");
      delay(1000);
      return;
    }

    // Run inference
    Serial.println("Running inference...");
    unsigned long start_time = millis();

    TfLiteStatus invoke_status = interpreter->Invoke();

    unsigned long inference_time = millis() - start_time;

    if (invoke_status != kTfLiteOk) {
      Serial.println("Error: Inference failed");
      delay(1000);
      return;
    }

    Serial.printf("Inference completed in %lu ms\n", inference_time);

    // Process output
    float* output_data = output_tensor->data.f;
    processModelOutput(output_data, outImg);

    // Send result via embedDIP
    convertTo(outImg);
    serial->send(outImg);

    Serial.println("Segmentation result sent!");

    // Wait for button release
    while (digitalRead(PIN_BUTTON) == LOW) {
      delay(10);
    }
  }

  delay(100);
}
