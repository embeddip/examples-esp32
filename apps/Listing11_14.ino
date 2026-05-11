#include "embedDIP.hpp"
#include "esp_heap_caps.h"
#include <Arduino.h>

#include "cnn_mnist_best_model.h"
#include "mnist_model.h"
#include "tlfm_classification_runner.h"

#define IMG_SIZE 28
#define IMG_PIXELS (IMG_SIZE * IMG_SIZE)

#ifndef PIN_BUTTON
#define PIN_BUTTON 15
#endif

constexpr int kTensorArenaSize = 4 * 1024 * 1024;

SET_LOOP_TASK_STACK_SIZE(32 * 1024);

const char *kModelName = "mnist";
const unsigned char *kModelData = mnist_model_tflite;
const unsigned int kModelDataLen = mnist_model_tflite_len;

// const char *kModelName = "cnn_mnist";
// const unsigned char *kModelData = cnn_mnist_best_tflite;
// const unsigned int kModelDataLen = cnn_mnist_best_tflite_len;

embedDIP::SerialDev serial(&esp32_uart);
embedDIP::Image inImg;
uint8_t *tensor_arena = nullptr;

void haltWithMessage(const char *message) {
  Serial.println(message);
  while (1) {
    delay(1000);
  }
}

void setup() {
  Serial.begin(115200);
  serial.init();
  pinMode(PIN_BUTTON, INPUT_PULLUP);

  inImg = embedDIP::Image(IMG_SIZE, IMG_SIZE, IMAGE_FORMAT_GRAYSCALE);

  tensor_arena = static_cast<uint8_t *>(heap_caps_malloc(
      kTensorArenaSize, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
  if (tensor_arena == nullptr) {
    haltWithMessage("Tensor arena allocation failed");
  }

  Serial.printf("Selected model: %s (%u bytes)\n", kModelName, kModelDataLen);

  if (tlfm_cls_init_with_arena(kModelData, kModelDataLen, tensor_arena,
                               kTensorArenaSize) != 0) {
    haltWithMessage("TFLM init failed");
  }
}

void loop() {
  if (digitalRead(PIN_BUTTON) == LOW) {
    serial.capture(inImg);

    const uint8_t *pixels = static_cast<const uint8_t *>(inImg.pixels());
    uint8_t class_id = 0;
    if (tlfm_cls_infer(pixels, IMG_PIXELS, &class_id) != 0) {
      haltWithMessage("TFLM infer failed");
    }

    serial.send1D(&class_id, sizeof(uint8_t), 1, SERIAL_DATA_OTHER);
  }

  delay(100);
}
