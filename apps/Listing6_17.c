#include "embedDIP.h"
#include <Arduino.h>

#ifndef PIN_BUTTON
#define PIN_BUTTON 15
#endif

SET_LOOP_TASK_STACK_SIZE(16 * 1024 * 2);

serial_t *serial = &esp32_uart;

Image *inImg  = nullptr;
Image *outImg = nullptr;

static const float sobelX_3x3[9] = {
   1,  0, -1,
   2,  0, -2,
   1,  0, -1
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
    filter2D(inImg, outImg, sobelX_3x3, 3);
    normalize(outImg);
    convertTo(outImg);
    serial->send(outImg);
  }
  delay(100);
}
