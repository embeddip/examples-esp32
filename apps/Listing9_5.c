#include "embedDIP.h"
#include <Arduino.h>

#ifndef PIN_BUTTON
#define PIN_BUTTON 15
#endif

SET_LOOP_TASK_STACK_SIZE(16 * 1024 * 2);

serial_t *serial = &esp32_uart;

Image *inImg = nullptr;
Image *hsiImg = nullptr;
Image *hsiREDImg = nullptr;
Image *hsiYELLOWImg = nullptr;
Image *hsiMergeImg = nullptr;
Image *outImg = nullptr;

void setup() {
  serial->init();

  createImage(IMAGE_RES_QVGA, IMAGE_FORMAT_RGB888, &inImg);
  createImage(IMAGE_RES_QVGA, IMAGE_FORMAT_HSI, &hsiImg);
  createImage(IMAGE_RES_QVGA, IMAGE_FORMAT_GRAYSCALE, &hsiREDImg);
  createImage(IMAGE_RES_QVGA, IMAGE_FORMAT_GRAYSCALE, &hsiYELLOWImg);
  createImage(IMAGE_RES_QVGA, IMAGE_FORMAT_GRAYSCALE, &hsiMergeImg);
  createImage(IMAGE_RES_QVGA, IMAGE_FORMAT_RGB888, &outImg);

  pinMode(PIN_BUTTON, INPUT_PULLUP);
}

void loop() {
  if (digitalRead(PIN_BUTTON) == LOW) {
    serial->capture(inImg);

    cvtColor(inImg, hsiImg, CVT_RGB888_TO_HSI);

    hueThreshold(hsiImg, hsiREDImg, 241, 14);
    hueThreshold(hsiImg, hsiYELLOWImg, 28, 42);

    _or(hsiREDImg, hsiYELLOWImg, hsiMergeImg);

    _and_(inImg, hsiMergeImg, outImg);

    serial->send(outImg);
  }
  delay(100);
}
