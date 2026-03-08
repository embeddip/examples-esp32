/**
 * unet Segmentation on ESP32 with embedDIP
 *
 * This example shows how to run unetV2 segmentation model
 * on ESP32 using TensorFlow Lite Micro and embedDIP for image capture
 *
 * Hardware: ESP32 with camera module
 * Libraries: TFLiteMicro (built-in), embedDIP
 */

#include "embedDIP.h"
#include "esp_heap_caps.h" // For ps_malloc
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/micro/system_setup.h"
#include "tensorflow/lite/schema/schema_generated.h"
#include <Arduino.h>

#define ps_malloc(size) heap_caps_malloc((size), MALLOC_CAP_SPIRAM)

// Include your converted TFLite model
#include "unet_model.h"

#ifndef PIN_BUTTON
#define PIN_BUTTON 15
#endif

// Model input/output dimensions (matching STM32 version)
#define INPUT_WIDTH 128
#define INPUT_HEIGHT 128
#define INPUT_CHANNELS 1  // Grayscale
#define OUTPUT_CHANNELS 2 // Segmentation classes

// TensorFlow Lite settings
// Increase by 16KB to cover the missing 7.7KB
constexpr int kTensorArenaSize = 1296 * 1024; // 1.296MB (was 1.28MB, needed +7.7KB)
uint8_t *tensor_arena = nullptr;

// Global variables
SET_LOOP_TASK_STACK_SIZE(8 * 1024); // Reduce from 32KB to 8KB to save PSRAM

serial_t *serial = &esp32_uart;
Image *inImg = nullptr;
Image *outImg = nullptr;
Image *confImg = nullptr; // Confidence map (optional)

const tflite::Model *model = nullptr;
tflite::MicroInterpreter *interpreter = nullptr;
TfLiteTensor *input_tensor = nullptr;
TfLiteTensor *output_tensor = nullptr;

bool prepareModelInput(Image *img, float *input_buffer) {
  uint8_t *pixel_data = (uint8_t *)img->pixels;
  for (int i = 0; i < INPUT_WIDTH * INPUT_HEIGHT; i++) {
    uint8_t pixel = pixel_data[i];
    input_buffer[i] = (float)pixel / 255.0f;
  }

  return true;
}

// Helper function: compute softmax for 2 classes
inline float softmax_confidence(float class0, float class1, bool use_class1) {
  float max_val = (class0 > class1) ? class0 : class1;
  float exp0 = expf(class0 - max_val);
  float exp1 = expf(class1 - max_val);
  float sum = exp0 + exp1;
  return use_class1 ? (exp1 / sum) : (exp0 / sum);
}

void processModelOutput(float *output_buffer, Image *outImg) {
  uint8_t *pixel_data = (uint8_t *)outImg->pixels;

  for (int i = 0; i < INPUT_WIDTH * INPUT_HEIGHT; i++) {
    float class0 = output_buffer[i * 2];
    float class1 = output_buffer[i * 2 + 1];

    // Option 1: Binary segmentation (current method)
    pixel_data[i] = (class1 > class0) ? 255 : 0;

    // Option 2: Confidence map - uncomment to use instead
    // bool predicted_class1 = (class1 > class0);
    // float confidence = softmax_confidence(class0, class1, predicted_class1);
    // pixel_data[i] = (uint8_t)(confidence * 255.0f);

    // Option 3: Difference-based confidence (faster, no exp)
    // float diff = fabsf(class1 - class0);
    // float sum = class0 + class1;
    // float confidence = (sum > 0) ? (diff / sum) : 0;
    // pixel_data[i] = (uint8_t)(confidence * 255.0f);
  }
}

// Process both segmentation and confidence (like STM32)
void processModelOutputWithConfidence(float *output_buffer, Image *segImg, Image *confImg) {
  uint8_t *seg_data = (uint8_t *)segImg->pixels;
  uint8_t *conf_data = (uint8_t *)confImg->pixels;

  for (int i = 0; i < INPUT_WIDTH * INPUT_HEIGHT; i++) {
    float class0 = output_buffer[i * 2];
    float class1 = output_buffer[i * 2 + 1];

    // Segmentation: argmax
    bool predicted_class1 = (class1 > class0);
    seg_data[i] = predicted_class1 ? 255 : 0;

    // Confidence: softmax probability of predicted class
    float confidence = softmax_confidence(class0, class1, predicted_class1);
    conf_data[i] = (uint8_t)(confidence * 255.0f);
  }
}

void setup() {
  setCpuFrequencyMhz(240);

  serial->init();

  // Allocate tensor arena FIRST (before images) to get maximum PSRAM
  tensor_arena = (uint8_t *)ps_malloc(kTensorArenaSize);

  createImageWH(INPUT_WIDTH, INPUT_HEIGHT, IMAGE_FORMAT_GRAYSCALE, &inImg);
  createImageWH(INPUT_WIDTH, INPUT_HEIGHT, IMAGE_FORMAT_GRAYSCALE, &outImg);
  // Uncomment to also send confidence map:
  createImageWH(INPUT_WIDTH, INPUT_HEIGHT, IMAGE_FORMAT_GRAYSCALE, &confImg);
  pinMode(PIN_BUTTON, INPUT_PULLUP);

  model = tflite::GetModel(unet_tflite);

  static tflite::MicroMutableOpResolver<21> resolver;

  // Operations for unetV2 segmentation
  resolver.AddConv2D();
  resolver.AddDepthwiseConv2D();
  resolver.AddTransposeConv();
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
  resolver.AddShape();
  resolver.AddConcatenation();
  resolver.AddMaxPool2D();
  resolver.AddStridedSlice();
  resolver.AddPack();

  static tflite::MicroInterpreter static_interpreter(
      model, resolver, tensor_arena, kTensorArenaSize);
  interpreter = &static_interpreter;

  TfLiteStatus allocate_status = interpreter->AllocateTensors();

  input_tensor = interpreter->input(0);
  output_tensor = interpreter->output(0);
}

void loop() {
  if (digitalRead(PIN_BUTTON) == LOW) {
    serial->capture(inImg);

    float *input_data = input_tensor->data.f;
    prepareModelInput(inImg, input_data);

    TfLiteStatus invoke_status = interpreter->Invoke();

    float *output_data = output_tensor->data.f;

    // Option A: Send only segmentation (current)
    processModelOutput(output_data, outImg);
    //convertTo(outImg);
    //serial->send(outImg);

    // Option B: Send segmentation + confidence (like STM32)
    // Uncomment if you created confImg in setup()
    processModelOutputWithConfidence(output_data, outImg, confImg);
    convertTo(outImg);
    serial->send(outImg);
    convertTo(confImg);
    serial->send(confImg);  // Send confidence map as second image
  }

  delay(100);
}
