#include "embedDIP.h"
#include "esp_heap_caps.h"
#include <Arduino.h>
#include <string.h>

#include "camvid_float32_model.h"
#include "camvid_int8_model.h"
#include "tlfm_camvid_runner.h"

#define IMG_SIZE 128
#define IMG_PIXELS (IMG_SIZE * IMG_SIZE)
#define OUT_CHANNELS 4
#define BACKGROUND_CLASS_ID 3

#ifndef PIN_BUTTON
#define PIN_BUTTON 15
#endif

constexpr int kTensorArenaSize = 4 * 1024 * 1024;

SET_LOOP_TASK_STACK_SIZE(32 * 1024);

const char *kModelName = "camvid_float32";
const unsigned char *kModelData = camvid_float32_tflite;
const unsigned int kModelDataLen = camvid_float32_tflite_len;

// const char *kModelName = "camvid_int8";
// const unsigned char *kModelData = camvid_int8_tflite;
// const unsigned int kModelDataLen = camvid_int8_tflite_len;

serial_t *serial = &esp32_uart;
Image *inImg = nullptr;
Image *classImg = nullptr;
Image *confImg = nullptr;
Image *visImg = nullptr;
Image *overlayImg = nullptr;
uint8_t *tensor_arena = nullptr;

uint8_t class_map[IMG_PIXELS] = {0};
uint8_t confidence_map[IMG_PIXELS] = {0};

static const uint8_t class_colors[OUT_CHANNELS][3] = {
    {255, 0, 0},
    {0, 255, 0},
    {255, 255, 0},
    {0, 0, 0},
};

void haltWithMessage(const char *message) {
  Serial.println(message);
  while (1) {
    delay(1000);
  }
}

void allocImages() {
  if (createImageWH(IMG_SIZE, IMG_SIZE, IMAGE_FORMAT_RGB888, &inImg) !=
          EMBEDDIP_OK ||
      createImageWH(IMG_SIZE, IMG_SIZE, IMAGE_FORMAT_GRAYSCALE, &classImg) !=
          EMBEDDIP_OK ||
      createImageWH(IMG_SIZE, IMG_SIZE, IMAGE_FORMAT_GRAYSCALE, &confImg) !=
          EMBEDDIP_OK ||
      createImageWH(IMG_SIZE, IMG_SIZE, IMAGE_FORMAT_RGB888, &visImg) !=
          EMBEDDIP_OK ||
      createImageWH(IMG_SIZE, IMG_SIZE, IMAGE_FORMAT_RGB888, &overlayImg) !=
          EMBEDDIP_OK) {
    haltWithMessage("Image allocation failed");
  }
}

void colorizeClassMap() {
  uint8_t *vis_pixels = static_cast<uint8_t *>(visImg->pixels);
  for (int i = 0; i < IMG_PIXELS; ++i) {
    const uint8_t cls =
        static_cast<uint8_t>((class_map[i] * (OUT_CHANNELS - 1) + 127) / 255);
    vis_pixels[3 * i + 0] = class_colors[cls][0];
    vis_pixels[3 * i + 1] = class_colors[cls][1];
    vis_pixels[3 * i + 2] = class_colors[cls][2];
  }
}

void makeOverlay(const uint8_t *rgb_pixels) {
  uint8_t *overlay_pixels = static_cast<uint8_t *>(overlayImg->pixels);
  for (int i = 0; i < IMG_PIXELS; ++i) {
    const uint8_t cls =
        static_cast<uint8_t>((class_map[i] * (OUT_CHANNELS - 1) + 127) / 255);

    const uint8_t src_r = rgb_pixels[3 * i + 0];
    const uint8_t src_g = rgb_pixels[3 * i + 1];
    const uint8_t src_b = rgb_pixels[3 * i + 2];

    if (cls == BACKGROUND_CLASS_ID) {
      overlay_pixels[3 * i + 0] = src_r;
      overlay_pixels[3 * i + 1] = src_g;
      overlay_pixels[3 * i + 2] = src_b;
    } else {
      overlay_pixels[3 * i + 0] = static_cast<uint8_t>(
          ((uint16_t)src_r * 55U + (uint16_t)class_colors[cls][0] * 45U) /
          100U);
      overlay_pixels[3 * i + 1] = static_cast<uint8_t>(
          ((uint16_t)src_g * 55U + (uint16_t)class_colors[cls][1] * 45U) /
          100U);
      overlay_pixels[3 * i + 2] = static_cast<uint8_t>(
          ((uint16_t)src_b * 55U + (uint16_t)class_colors[cls][2] * 45U) /
          100U);
    }
  }
}

void setup() {
  Serial.begin(115200);
  serial->init();
  pinMode(PIN_BUTTON, INPUT_PULLUP);

  allocImages();

  tensor_arena = static_cast<uint8_t *>(heap_caps_malloc(
      kTensorArenaSize, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
  if (tensor_arena == nullptr) {
    haltWithMessage("Tensor arena allocation failed");
  }

  Serial.printf("Selected model: %s (%u bytes)\n", kModelName, kModelDataLen);

  if (tlfm_camvid_init_with_arena(kModelData, kModelDataLen, tensor_arena,
                                  kTensorArenaSize) != 0) {
    haltWithMessage("TFLM init failed");
  }
}

void loop() {
  if (digitalRead(PIN_BUTTON) == LOW) {
    serial->capture(inImg);

    const uint8_t *rgb_pixels = static_cast<const uint8_t *>(inImg->pixels);
    if (tlfm_camvid_infer(rgb_pixels, class_map, confidence_map) != 0) {
      haltWithMessage("TFLM infer failed");
    }

    memcpy(classImg->pixels, class_map, IMG_PIXELS);
    memcpy(confImg->pixels, confidence_map, IMG_PIXELS);
    colorizeClassMap();
    makeOverlay(rgb_pixels);

    serial->send(inImg);
    serial->send(classImg);
    serial->send(confImg);
    serial->send(visImg);
    serial->send(overlayImg);
  }

  delay(100);
}
