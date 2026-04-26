#include "embedDIP.h"
#include "esp_heap_caps.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/micro/system_setup.h"
#include "tensorflow/lite/schema/schema_generated.h"
#include <Arduino.h>

#define ps_malloc(size) heap_caps_malloc((size), MALLOC_CAP_SPIRAM)

#include "mobilenetv2_fp32_model.h"

#ifndef PIN_BUTTON
#define PIN_BUTTON 15
#endif

#define INPUT_WIDTH 128
#define INPUT_HEIGHT 128
#define INPUT_CHANNELS 1
#define OUTPUT_CHANNELS 2

constexpr int kTensorArenaSize = 1600 * 1024;
uint8_t *tensor_arena = nullptr;

SET_LOOP_TASK_STACK_SIZE(32 * 1024);

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
    input_buffer[i] = (float)pixel_data[i] / 255.0f;
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
  serial->init();

  createImageWH(INPUT_WIDTH, INPUT_HEIGHT, IMAGE_FORMAT_GRAYSCALE, &inImg);
  createImageWH(INPUT_WIDTH, INPUT_HEIGHT, IMAGE_FORMAT_GRAYSCALE, &outImg);
  pinMode(PIN_BUTTON, INPUT_PULLUP);

  tensor_arena = (uint8_t *)ps_malloc(kTensorArenaSize);

  model = tflite::GetModel(mobilenetv2_fp32_tflite);

  static tflite::MicroMutableOpResolver<17> resolver;
  resolver.AddConv2D();
  resolver.AddDepthwiseConv2D();
  resolver.AddTransposeConv();
  resolver.AddReshape();
  resolver.AddSoftmax();
  resolver.AddMaxPool2D();
  resolver.AddRelu();
  resolver.AddAdd();
  resolver.AddMul();
  resolver.AddQuantize();
  resolver.AddDequantize();
  resolver.AddConcatenation();
  resolver.AddPad();
  resolver.AddResizeBilinear();
  resolver.AddShape();
  resolver.AddStridedSlice();
  resolver.AddPack();

  static tflite::MicroInterpreter static_interpreter(
      model, resolver, tensor_arena, kTensorArenaSize);
  interpreter = &static_interpreter;

  TfLiteStatus allocate_status = interpreter->AllocateTensors();
  if (allocate_status != kTfLiteOk) {
    while (1);
  }

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
