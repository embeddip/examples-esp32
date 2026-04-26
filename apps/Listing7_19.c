#include "embedDIP.h"
#include <Arduino.h>

#ifndef PIN_BUTTON
#define PIN_BUTTON 15
#endif

SET_LOOP_TASK_STACK_SIZE(16 * 1024 * 2);

serial_t *serial = &esp32_uart;

Image *inImg = nullptr;
Image *outImg = nullptr;

static float lp3[3] = {1.0f, 1.0f, 1.0f};

void setup() {
  serial->init();
  createImage(IMAGE_RES_QVGA, IMAGE_FORMAT_GRAYSCALE, &inImg);
  createImage(IMAGE_RES_QVGA, IMAGE_FORMAT_GRAYSCALE, &outImg);
  pinMode(PIN_BUTTON, INPUT_PULLUP);
}

void loop() {
  if (digitalRead(PIN_BUTTON) == LOW) {
    serial->capture(inImg);
    sepfilter2D(inImg, outImg, 3, lp3, 3, lp3, 1.0f / 9.0f);
    convertTo(outImg);
    serial->send(outImg);
  }
  delay(100);
}
