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
#include "esp_heap_caps.h" // For ps_malloc
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/micro/system_setup.h"
#include "tensorflow/lite/schema/schema_generated.h"
#include <Arduino.h>

#define ps_malloc(size) heap_caps_malloc((size), MALLOC_CAP_SPIRAM)

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
// Model needs ~696KB, so allocate 800KB for safety (all in PSRAM)
constexpr int kTensorArenaSize = 800 * 1024; // 800KB arena in PSRAM
uint8_t *tensor_arena = nullptr;

// Global variables
SET_LOOP_TASK_STACK_SIZE(16 * 1024 * 2);

serial_t *serial = &esp32_uart;
Image *inImg = nullptr;
Image *outImg = nullptr;

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

void processModelOutput(float *output_buffer, Image *outImg) {

  uint8_t *pixel_data = (uint8_t *)outImg->pixels;
  for (int i = 0; i < INPUT_WIDTH * INPUT_HEIGHT; i++) {
    float class0 = output_buffer[i * 2];
    float class1 = output_buffer[i * 2 + 1];

    pixel_data[i] = (class1 > class0) ? 255 : 0;
  }
}

void setup() {
  setCpuFrequencyMhz(240);

  serial->init();

  createImageWH(INPUT_WIDTH, INPUT_HEIGHT, IMAGE_FORMAT_GRAYSCALE, &inImg);
  createImageWH(INPUT_WIDTH, INPUT_HEIGHT, IMAGE_FORMAT_GRAYSCALE, &outImg);
  pinMode(PIN_BUTTON, INPUT_PULLUP);

  tensor_arena = (uint8_t *)ps_malloc(kTensorArenaSize);

  model = tflite::GetModel(mobilenetv2_segmentation_tflite);

  static tflite::MicroMutableOpResolver<21> resolver;

  // Operations for MobileNetV2 segmentation
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
    processModelOutput(output_data, outImg);

    convertTo(outImg);
    serial->send(outImg);
  }

  delay(100);
}
