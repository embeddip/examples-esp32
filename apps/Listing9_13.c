#include "embedDIP.h"
#include <Arduino.h>

#ifndef PIN_BUTTON
#define PIN_BUTTON 15
#endif

SET_LOOP_TASK_STACK_SIZE(16 * 1024 * 2);

serial_t *serial = &esp32_uart;

Image *inImg = nullptr;
Image *outImg = nullptr;

void setup() {
  serial->init();
  createImage(IMAGE_RES_QVGA, IMAGE_FORMAT_RGB888, &inImg);
  createImage(IMAGE_RES_QVGA, IMAGE_FORMAT_RGB888, &outImg);
  pinMode(PIN_BUTTON, INPUT_PULLUP);
}

void loop() {
  if (digitalRead(PIN_BUTTON) == LOW) {
    serial->capture(inImg);

    // Color K-Means (K=3). Uncomment for K=5.
    colorKMeans(inImg, outImg, 3);
    // colorKMeans(inImg, outImg, 5);

    serial->send(outImg);
  }
  delay(100);
}
