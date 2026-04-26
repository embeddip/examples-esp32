#include "embedDIP.hpp"
#include "esp_heap_caps.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/micro/system_setup.h"
#include "tensorflow/lite/schema/schema_generated.h"
#include <Arduino.h>

#define ps_malloc(size) heap_caps_malloc((size), MALLOC_CAP_SPIRAM)

#include "cnn_mnist_best_model.h"

#ifndef PIN_BUTTON
#define PIN_BUTTON 15
#endif

#define INPUT_WIDTH 28
#define INPUT_HEIGHT 28
#define INPUT_SIZE (INPUT_WIDTH * INPUT_HEIGHT)
#define NUM_CLASSES 10

constexpr int kTensorArenaSize = 800 * 1024;
uint8_t *tensor_arena = nullptr;

SET_LOOP_TASK_STACK_SIZE(32 * 1024);

embedDIP::SerialDev serial(&esp32_uart);

embedDIP::Image inImg;

const tflite::Model *model = nullptr;
tflite::MicroInterpreter *interpreter = nullptr;
TfLiteTensor *input_tensor = nullptr;
TfLiteTensor *output_tensor = nullptr;

void setup() {
  serial.init();

  inImg = embedDIP::Image(INPUT_WIDTH, INPUT_HEIGHT, IMAGE_FORMAT_GRAYSCALE);

  pinMode(PIN_BUTTON, INPUT_PULLUP);

  tensor_arena = (uint8_t *)ps_malloc(kTensorArenaSize);

  model = tflite::GetModel(cnn_mnist_best_tflite);

  static tflite::MicroMutableOpResolver<10> resolver;
  resolver.AddConv2D();
  resolver.AddMaxPool2D();
  resolver.AddFullyConnected();
  resolver.AddReshape();
  resolver.AddShape();
  resolver.AddStridedSlice();
  resolver.AddPack();
  resolver.AddSoftmax();
  resolver.AddRelu();
  resolver.AddQuantize();

  static tflite::MicroInterpreter static_interpreter(
      model, resolver, tensor_arena, kTensorArenaSize);
  interpreter = &static_interpreter;

  TfLiteStatus allocate_status = interpreter->AllocateTensors();

  input_tensor = interpreter->input(0);
  output_tensor = interpreter->output(0);
}

void loop() {
  if (digitalRead(PIN_BUTTON) == LOW) {
    serial.capture(inImg);

    float *input_data = input_tensor->data.f;

    uint8_t *pixel_data = (uint8_t *)inImg.pixels();

    for (int i = 0; i < INPUT_SIZE; i++) {
      input_data[i] = (float)pixel_data[i] / 255.0f;
    }

    TfLiteStatus invoke_status = interpreter->Invoke();

    float *output_data = output_tensor->data.f;
    int predicted_digit = 0;
    float max_score = output_data[0];

    for (int i = 1; i < NUM_CLASSES; i++) {
      if (output_data[i] > max_score) {
        max_score = output_data[i];
        predicted_digit = i;
      }
    }

    serial.send1D(&predicted_digit, sizeof(int), 1, SERIAL_DATA_OTHER);
  }

  delay(100);
}
