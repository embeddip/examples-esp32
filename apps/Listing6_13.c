#include "embedDIP.h"
#include <Arduino.h>

const int PIN_BUTTON = 15;
SET_LOOP_TASK_STACK_SIZE(16 * 1024 * 2);

serial_t *serial = &esp32_uart;

Image *srcImg = nullptr;
Image *refImg = nullptr;
Image *outImg = nullptr;

static int refHistogram[256] = {0};

void setup() {
  serial->init();

  createImage(IMAGE_RES_QVGA, IMAGE_FORMAT_GRAYSCALE, &srcImg);
  createImage(IMAGE_RES_QVGA, IMAGE_FORMAT_GRAYSCALE, &refImg);
  createImage(IMAGE_RES_QVGA, IMAGE_FORMAT_GRAYSCALE, &outImg);

  pinMode(PIN_BUTTON, INPUT_PULLUP);
}

void loop() {
  if (digitalRead(PIN_BUTTON) == LOW) {

    serial->capture(refImg);
    histForm(refImg, refHistogram);

    serial->capture(srcImg);
    histSpec(srcImg, outImg, refHistogram);
    convertTo(outImg);
    serial->send(outImg);
  }
  delay(100);
}
