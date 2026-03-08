#include "embedDIP.h"
#include <Arduino.h>

#ifndef PIN_BUTTON
#define PIN_BUTTON 15
#endif

SET_LOOP_TASK_STACK_SIZE(16 * 1024 * 2);

serial_t *serial = &esp32_uart;

Image *img = nullptr;
Image *outImg = nullptr;
Image *rImg = nullptr;
Image *gImg = nullptr;
Image *bImg = nullptr;
Image *rImgOut = nullptr;
Image *gImgOut = nullptr;
Image *bImgOut = nullptr;

static const float lowPassFilter3x3[9] = {
    1.0f / 9.0f, 1.0f / 9.0f, 1.0f / 9.0f,
    1.0f / 9.0f, 1.0f / 9.0f, 1.0f / 9.0f,
    1.0f / 9.0f, 1.0f / 9.0f, 1.0f / 9.0f};

void setup() {
  serial->init();

  createImage(IMAGE_RES_QVGA, IMAGE_FORMAT_RGB888, &img);
  createImage(IMAGE_RES_QVGA, IMAGE_FORMAT_RGB888, &outImg);

  createImage(IMAGE_RES_QVGA, IMAGE_FORMAT_GRAYSCALE, &rImg);
  createImage(IMAGE_RES_QVGA, IMAGE_FORMAT_GRAYSCALE, &gImg);
  createImage(IMAGE_RES_QVGA, IMAGE_FORMAT_GRAYSCALE, &bImg);

  createImage(IMAGE_RES_QVGA, IMAGE_FORMAT_GRAYSCALE, &rImgOut);
  createImage(IMAGE_RES_QVGA, IMAGE_FORMAT_GRAYSCALE, &gImgOut);
  createImage(IMAGE_RES_QVGA, IMAGE_FORMAT_GRAYSCALE, &bImgOut);

  pinMode(PIN_BUTTON, INPUT_PULLUP);
}

void loop() {
  if (digitalRead(PIN_BUTTON) == LOW) {
    serial->capture(img);

    rgbSplit(img, rImg, gImg, bImg);

    filter2D(rImg, rImgOut, lowPassFilter3x3, 3);
    filter2D(gImg, gImgOut, lowPassFilter3x3, 3);
    filter2D(bImg, bImgOut, lowPassFilter3x3, 3);

    convertTo(rImgOut);
    convertTo(gImgOut);
    convertTo(bImgOut);

    rgbMerge(rImgOut, gImgOut, bImgOut, outImg);

    serial->send(outImg);
  }
  delay(100);
}
