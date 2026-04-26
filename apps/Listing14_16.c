#include "embedDIP.h"
#include "esp_heap_caps.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/schema/schema_generated.h"
#include <Arduino.h>

#define ps_malloc(size) heap_caps_malloc((size), MALLOC_CAP_SPIRAM)

#include "camvid_int8_model.h"

#ifndef PIN_BUTTON
#define PIN_BUTTON 15
#endif

#define INPUT_WIDTH 128
#define INPUT_HEIGHT 128
#define INPUT_CHANNELS 3
#define OUTPUT_CHANNELS 4
#define PIXELS (INPUT_WIDTH * INPUT_HEIGHT)
#define BACKGROUND_CLASS_ID 3

constexpr int kTensorArenaSize = 2 * 1024 * 1024;
uint8_t *tensor_arena = nullptr;

SET_LOOP_TASK_STACK_SIZE(16 * 1024 * 2);

serial_t *serial = &esp32_uart;
Image *inImg = nullptr;
Image *grayImg = nullptr;
Image *visImg = nullptr;
Image *overlayImg = nullptr;

const tflite::Model *model = nullptr;
tflite::MicroInterpreter *interpreter = nullptr;
TfLiteTensor *input_tensor = nullptr;
TfLiteTensor *output_tensor = nullptr;

static uint8_t class_id_map[PIXELS];
static uint8_t class_index_map[PIXELS];
static uint8_t confidence_map[PIXELS];

static const uint8_t class_colors[OUTPUT_CHANNELS][3] = {
    {255, 0, 0},
    {0, 255, 0},
    {255, 255, 0},
    {0, 0, 0}
};

bool prepareModelInput(Image *img, TfLiteTensor *input) {
  uint8_t *pixel_data = (uint8_t *)img->pixels;
  const int n = PIXELS * INPUT_CHANNELS;

  if (input->type == kTfLiteInt8) {
    int8_t *dst = input->data.int8;
    for (int i = 0; i < n; ++i) {
      dst[i] = (int8_t)((int16_t)pixel_data[i] - 128);
    }
    return true;
  }

  if (input->type == kTfLiteUInt8) {
    uint8_t *dst = input->data.uint8;
    memcpy(dst, pixel_data, n);
    return true;
  }

  if (input->type == kTfLiteFloat32) {
    float *dst = input->data.f;
    for (int i = 0; i < n; ++i) {
      dst[i] = (float)pixel_data[i] / 255.0f;
    }
    return true;
  }

  return false;
}

bool processModelOutput(const TfLiteTensor *output) {
  if (output->type == kTfLiteInt8) {
    const int8_t *src = output->data.int8;
    for (int i = 0; i < PIXELS; ++i) {
      int base = i * OUTPUT_CHANNELS;
      int best_class = 0;
      int8_t best_score = src[base];

      for (int c = 1; c < OUTPUT_CHANNELS; ++c) {
        int8_t score = src[base + c];
        if (score > best_score) {
          best_score = score;
          best_class = c;
        }
      }

      class_id_map[i] = (uint8_t)best_class;
      class_index_map[i] = (uint8_t)((best_class * 255) / (OUTPUT_CHANNELS - 1));
      confidence_map[i] = (uint8_t)((int16_t)best_score + 128);
    }
    return true;
  }

  if (output->type == kTfLiteFloat32) {
    const float *src = output->data.f;
    for (int i = 0; i < PIXELS; ++i) {
      int base = i * OUTPUT_CHANNELS;
      int best_class = 0;
      float best_score = src[base];

      for (int c = 1; c < OUTPUT_CHANNELS; ++c) {
        float score = src[base + c];
        if (score > best_score) {
          best_score = score;
          best_class = c;
        }
      }

      class_id_map[i] = (uint8_t)best_class;
      class_index_map[i] = (uint8_t)((best_class * 255) / (OUTPUT_CHANNELS - 1));
      if (best_score < 0.0f) best_score = 0.0f;
      if (best_score > 1.0f) best_score = 1.0f;
      confidence_map[i] = (uint8_t)(best_score * 255.0f);
    }
    return true;
  }

  return false;
}

void buildVisualizationImages() {
  uint8_t *vis_pixels = (uint8_t *)visImg->pixels;
  uint8_t *overlay_pixels = (uint8_t *)overlayImg->pixels;
  uint8_t *src_pixels = (uint8_t *)inImg->pixels;

  for (int i = 0; i < PIXELS; ++i) {
    uint8_t cls = class_id_map[i];

    vis_pixels[3 * i + 0] = class_colors[cls][0];
    vis_pixels[3 * i + 1] = class_colors[cls][1];
    vis_pixels[3 * i + 2] = class_colors[cls][2];

    uint8_t src_r = src_pixels[3 * i + 0];
    uint8_t src_g = src_pixels[3 * i + 1];
    uint8_t src_b = src_pixels[3 * i + 2];

    if (cls == BACKGROUND_CLASS_ID) {
      overlay_pixels[3 * i + 0] = src_r;
      overlay_pixels[3 * i + 1] = src_g;
      overlay_pixels[3 * i + 2] = src_b;
    } else {
      uint8_t mask_r = class_colors[cls][0];
      uint8_t mask_g = class_colors[cls][1];
      uint8_t mask_b = class_colors[cls][2];

      overlay_pixels[3 * i + 0] = (uint8_t)(((uint16_t)src_r * 55U + (uint16_t)mask_r * 45U) / 100U);
      overlay_pixels[3 * i + 1] = (uint8_t)(((uint16_t)src_g * 55U + (uint16_t)mask_g * 45U) / 100U);
      overlay_pixels[3 * i + 2] = (uint8_t)(((uint16_t)src_b * 55U + (uint16_t)mask_b * 45U) / 100U);
    }
  }
}

void setup() {
  setCpuFrequencyMhz(240);
  serial->init();
  pinMode(PIN_BUTTON, INPUT_PULLUP);

  createImageWH(INPUT_WIDTH, INPUT_HEIGHT, IMAGE_FORMAT_RGB888, &inImg);
  createImageWH(INPUT_WIDTH, INPUT_HEIGHT, IMAGE_FORMAT_GRAYSCALE, &grayImg);
  createImageWH(INPUT_WIDTH, INPUT_HEIGHT, IMAGE_FORMAT_RGB888, &visImg);
  createImageWH(INPUT_WIDTH, INPUT_HEIGHT, IMAGE_FORMAT_RGB888, &overlayImg);

  tensor_arena = (uint8_t *)ps_malloc(kTensorArenaSize);

  model = tflite::GetModel(camvid_int8_tflite);

  static tflite::MicroMutableOpResolver<21> resolver;
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

  static tflite::MicroInterpreter static_interpreter(model, resolver, tensor_arena, kTensorArenaSize);
  interpreter = &static_interpreter;

  interpreter->AllocateTensors();

  input_tensor = interpreter->input(0);
  output_tensor = interpreter->output(0);
}

void loop() {
  if (digitalRead(PIN_BUTTON) == LOW) {
    serial->capture(inImg);

    prepareModelInput(inImg, input_tensor);

    interpreter->Invoke();

    processModelOutput(output_tensor);

    buildVisualizationImages();

    serial->send(inImg);
    memcpy(grayImg->pixels, class_index_map, PIXELS);
    serial->send(grayImg);
    memcpy(grayImg->pixels, confidence_map, PIXELS);
    serial->send(grayImg);
    serial->send(visImg);
    serial->send(overlayImg);

    while (digitalRead(PIN_BUTTON) == LOW) {
      delay(10);
    }
  }

  delay(100);
}
