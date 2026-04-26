#include "embedDIP.h"
#include <Arduino.h>

const int PIN_BUTTON = 15;
SET_LOOP_TASK_STACK_SIZE(16 * 1024 * 2);

serial_t *serial = &esp32_uart;

Image *inImg  = nullptr;
Image *outImg = nullptr;

static const float lowPassFilter3x3[9] = {
  1.0f/9.0f, 1.0f/9.0f, 1.0f/9.0f,
  1.0f/9.0f, 1.0f/9.0f, 1.0f/9.0f,
  1.0f/9.0f, 1.0f/9.0f, 1.0f/9.0f
};

static const float lowPassFilter5x5[25] = {
  1.0f/25.0f, 1.0f/25.0f, 1.0f/25.0f, 1.0f/25.0f, 1.0f/25.0f,
  1.0f/25.0f, 1.0f/25.0f, 1.0f/25.0f, 1.0f/25.0f, 1.0f/25.0f,
  1.0f/25.0f, 1.0f/25.0f, 1.0f/25.0f, 1.0f/25.0f, 1.0f/25.0f,
  1.0f/25.0f, 1.0f/25.0f, 1.0f/25.0f, 1.0f/25.0f, 1.0f/25.0f,
  1.0f/25.0f, 1.0f/25.0f, 1.0f/25.0f, 1.0f/25.0f, 1.0f/25.0f
};

void setup() {
  serial->init();

  createImage(IMAGE_RES_QVGA, IMAGE_FORMAT_GRAYSCALE, &inImg);
  createImage(IMAGE_RES_QVGA, IMAGE_FORMAT_GRAYSCALE, &outImg);

  pinMode(PIN_BUTTON, INPUT_PULLUP);
}

void loop() {
  if (digitalRead(PIN_BUTTON) == LOW) {
    serial->capture(inImg);

    filter2D(inImg, outImg, lowPassFilter3x3, 3);

    convertTo(outImg);
    serial->send(outImg);

    filter2D(inImg, outImg, lowPassFilter5x5, 5);

    convertTo(outImg);
    serial->send(outImg);
  }
  delay(100);
}
