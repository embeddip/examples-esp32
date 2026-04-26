#line 1 "/media/odurgut/82a56851-6a27-45e3-afb0-3549236924ca/sda1/book/arduinotest/examples/mytest/mytest.ino"
#include "embedDIP.hpp"
#include "esp_heap_caps.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/micro/system_setup.h"
#include "tensorflow/lite/schema/schema_generated.h"
#include <Arduino.h>

#define ps_malloc(size) heap_caps_malloc((size), MALLOC_CAP_SPIRAM)

#include "unet_fp32_data.h"

#ifndef PIN_BUTTON
#define PIN_BUTTON 15
#endif

#define INPUT_WIDTH 128
#define INPUT_HEIGHT 128
#define INPUT_CHANNELS 1
#define OUTPUT_CHANNELS 2

// U-Net FP32 needs roughly >=2 MB arena on ESP32; keep extra headroom.
constexpr int kTensorArenaSize = 2304 * 1024;
uint8_t *tensor_arena = nullptr;

#line 26 "/media/odurgut/82a56851-6a27-45e3-afb0-3549236924ca/sda1/book/arduinotest/examples/mytest/mytest.ino"
size_t getArduinoLoopTaskStackSize();
#line 38 "/media/odurgut/82a56851-6a27-45e3-afb0-3549236924ca/sda1/book/arduinotest/examples/mytest/mytest.ino"
bool prepareModelInput(embedDIP::Image &img, float *input_buffer);
#line 46 "/media/odurgut/82a56851-6a27-45e3-afb0-3549236924ca/sda1/book/arduinotest/examples/mytest/mytest.ino"
void processModelOutput(float *output_buffer, embedDIP::Image &outImg);
#line 57 "/media/odurgut/82a56851-6a27-45e3-afb0-3549236924ca/sda1/book/arduinotest/examples/mytest/mytest.ino"
void setup();
#line 111 "/media/odurgut/82a56851-6a27-45e3-afb0-3549236924ca/sda1/book/arduinotest/examples/mytest/mytest.ino"
void loop();
#line 26 "/media/odurgut/82a56851-6a27-45e3-afb0-3549236924ca/sda1/book/arduinotest/examples/mytest/mytest.ino"
SET_LOOP_TASK_STACK_SIZE(32 * 1024);

embedDIP::SerialDev serial(&esp32_uart);

embedDIP::Image inImg;
embedDIP::Image outImg;

const tflite::Model *model = nullptr;
tflite::MicroInterpreter *interpreter = nullptr;
TfLiteTensor *input_tensor = nullptr;
TfLiteTensor *output_tensor = nullptr;

bool prepareModelInput(embedDIP::Image &img, float *input_buffer) {
  uint8_t *pixel_data = (uint8_t *)img.pixels();
  for (int i = 0; i < INPUT_WIDTH * INPUT_HEIGHT; i++) {
    input_buffer[i] = (float)pixel_data[i] / 255.0f;
  }
  return true;
}

void processModelOutput(float *output_buffer, embedDIP::Image &outImg) {
  uint8_t *pixel_data = (uint8_t *)outImg.pixels();

  for (int i = 0; i < INPUT_WIDTH * INPUT_HEIGHT; i++) {
    float class0 = output_buffer[i * 2];
    float class1 = output_buffer[i * 2 + 1];

    pixel_data[i] = (class1 > class0) ? 255 : 0;
  }
}

void setup() {
  Serial.begin(115200);
  serial.init();

  inImg = embedDIP::Image(INPUT_WIDTH, INPUT_HEIGHT, IMAGE_FORMAT_GRAYSCALE);
  outImg = embedDIP::Image(INPUT_WIDTH, INPUT_HEIGHT, IMAGE_FORMAT_GRAYSCALE);

  pinMode(PIN_BUTTON, INPUT_PULLUP);

  tensor_arena = (uint8_t *)heap_caps_malloc(
      kTensorArenaSize, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  if (tensor_arena == nullptr) {
    Serial.printf("Failed to allocate tensor arena (%d bytes)\n", kTensorArenaSize);
    while (1) {
      delay(1000);
    }
  }

  model = tflite::GetModel(g_unet_fp32_data);

  static tflite::MicroMutableOpResolver<17> resolver;
  resolver.AddConv2D();
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
    Serial.println("AllocateTensors() failed");
    while (1) {
      delay(1000);
    }
  }

  input_tensor = interpreter->input(0);
  output_tensor = interpreter->output(0);
}

void loop() {
  if (digitalRead(PIN_BUTTON) == LOW) {
    serial.capture(inImg);

    float *input_data = input_tensor->data.f;
    prepareModelInput(inImg, input_data);

    TfLiteStatus invoke_status = interpreter->Invoke();

    float *output_data = output_tensor->data.f;
    processModelOutput(output_data, outImg);

    outImg.convertTo();
    serial.send(outImg);
  }

  delay(100);
}

