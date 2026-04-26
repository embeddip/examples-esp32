#include "embedDIP.h"
#include <Arduino.h>

#ifndef PIN_BUTTON
#define PIN_BUTTON 15
#endif

SET_LOOP_TASK_STACK_SIZE(16 * 1024 * 2);

serial_t *serial = &esp32_uart;

Image *inImg = NULL;
Image *outImg = NULL;
Image *maskImg = NULL;

void setup() {
  createImage(IMAGE_RES_QVGA, IMAGE_FORMAT_GRAYSCALE, &inImg);
  createImage(IMAGE_RES_QVGA, IMAGE_FORMAT_GRAYSCALE, &outImg);
  createImage(IMAGE_RES_QVGA, IMAGE_FORMAT_MASK, &maskImg);

  serial->init();
  pinMode(PIN_BUTTON, INPUT_PULLUP);
}

void loop() {
  if (digitalRead(PIN_BUTTON) == LOW) {
    serial->capture(inImg);

    Rectangle roi;
    roi.x = 193;
    roi.y = 24;
    roi.width = 241;
    roi.height = 244;

    grabCutLite(inImg, maskImg, roi, 1);

    convertTo(maskImg);
    serial->send(maskImg);

    _and_(inImg, maskImg, outImg);
    serial->send(outImg);
  }
  delay(100);
}
