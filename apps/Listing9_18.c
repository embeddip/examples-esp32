#include "embedDIP.h"
#include <Arduino.h>

#ifndef PIN_BUTTON
#define PIN_BUTTON 15
#endif

SET_LOOP_TASK_STACK_SIZE(16 * 1024 * 2);

serial_t *serial = &esp32_uart;

Image *inImg = nullptr;
Image *sX = nullptr;
Image *sY = nullptr;
Image *outImg = nullptr;

static float kernelX[9] = {-1, 0, 1, -2, 0, 2, -1, 0, 1};

static float kernelY[9] = {1, 2, 1, 0, 0, 0, -1, -2, -1};

void setup() {
  serial->init();
  createImage(IMAGE_RES_QVGA, IMAGE_FORMAT_GRAYSCALE, &inImg);
  createImage(IMAGE_RES_QVGA, IMAGE_FORMAT_GRAYSCALE, &sX);
  createImage(IMAGE_RES_QVGA, IMAGE_FORMAT_GRAYSCALE, &sY);
  createImage(IMAGE_RES_QVGA, IMAGE_FORMAT_GRAYSCALE, &outImg);
  pinMode(PIN_BUTTON, INPUT_PULLUP);
}

void loop() {
  if (digitalRead(PIN_BUTTON) == LOW) {
    serial->capture(inImg);

    // Sobel X
    filter2D(inImg, sX, kernelX, 3);
    convertTo(sX);
    serial->send(sX);

    // Sobel Y
    filter2D(inImg, sY, kernelY, 3);
    convertTo(sY);
    serial->send(sY);

    // Gradient magnitude (|grad| from sX, sY)
    gradientMagnitude(sX, sY, outImg);
    convertTo(outImg);
    serial->send(outImg);
  }
  delay(100);
}
