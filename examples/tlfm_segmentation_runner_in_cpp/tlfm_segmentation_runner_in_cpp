#include "embedDIP.hpp"
#include "esp_heap_caps.h"
#include <Arduino.h>

#include "tlfm_segmentation_runner.h"
// Select exactly one model header and one model data block below.
// #include "unet_fp32_model.h"
// #include "unet_s_fp32_model.h"
#include "mobilenet_fp32_model.h"

#ifndef PIN_BUTTON
#define PIN_BUTTON 15
#endif

constexpr int kTensorArenaSize = 2304 * 1024;

SET_LOOP_TASK_STACK_SIZE(32 * 1024);

// U-Net
// const char *kModelName = "unet_fp32";
// const unsigned char *kModelData = g_model_01_unet_fp32_data;
// const unsigned int kModelDataLen = g_model_01_unet_fp32_data_len;

// Simplified U-Net
// const char *kModelName = "unet_s_fp32";
// const unsigned char *kModelData = g_model_02_unet_s_fp32_data;
// const unsigned int kModelDataLen = g_model_02_unet_s_fp32_data_len;

// MobileNetV2 based segmentation
const char *kModelName = "mobilenet_fp32";
const unsigned char *kModelData = g_model_03_best_unet_fp32_data;
const unsigned int kModelDataLen = g_model_03_best_unet_fp32_data_len;

embedDIP::SerialDev serial(&esp32_uart);
embedDIP::Image inImg;
embedDIP::Image maskImg;
uint8_t *tensor_arena = nullptr;

void haltWithMessage(const char *message)
{
  Serial.println(message);
  while (1)
  {
    delay(1000);
  }
}

void setup()
{
  Serial.begin(115200);
  serial.init();
  pinMode(PIN_BUTTON, INPUT_PULLUP);

  inImg = embedDIP::Image(TLFM_SEG_INPUT_WIDTH, TLFM_SEG_INPUT_HEIGHT,
                          IMAGE_FORMAT_GRAYSCALE);
  maskImg = embedDIP::Image(TLFM_SEG_INPUT_WIDTH, TLFM_SEG_INPUT_HEIGHT,
                            IMAGE_FORMAT_GRAYSCALE);

  tensor_arena = static_cast<uint8_t *>(heap_caps_malloc(
      kTensorArenaSize, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
  if (tensor_arena == nullptr)
  {
    haltWithMessage("Tensor arena allocation failed");
  }

  Serial.printf("Selected model: %s (%u bytes)\n", kModelName, kModelDataLen);

  if (tlfm_seg_init_with_arena(kModelData, kModelDataLen, tensor_arena,
                               kTensorArenaSize) != 0)
  {
    haltWithMessage("TFLM init failed");
  }
}

void loop()
{
  if (digitalRead(PIN_BUTTON) == LOW)
  {
    serial.capture(inImg);

    const uint8_t *pixels = static_cast<const uint8_t *>(inImg.pixels());
    uint8_t *mask = static_cast<uint8_t *>(maskImg.pixels());

    if (tlfm_seg_infer(pixels, mask) != 0)
    {
      haltWithMessage("TFLM infer failed");
    }

    maskImg.convertTo();
    serial.send(maskImg);
  }

  delay(100);
}
