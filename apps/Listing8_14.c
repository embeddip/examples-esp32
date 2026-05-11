#include "embedDIP.h"
#include <Arduino.h>

#ifndef PIN_BUTTON
#define PIN_BUTTON 15
#endif

SET_LOOP_TASK_STACK_SIZE(16 * 1024 * 2);

serial_t *serial = &esp32_uart;

Image *inImg = NULL;
Image *outImg = NULL;

void setup() {
  createImage(IMAGE_RES_QVGA, IMAGE_FORMAT_RGB888, &inImg);
  createImage(IMAGE_RES_QVGA, IMAGE_FORMAT_RGB888, &outImg);

  serial->init();
  pinMode(PIN_BUTTON, INPUT_PULLUP);
}

void loop() {
  if (digitalRead(PIN_BUTTON) == LOW) {
    serial->capture(inImg);

    // Color region growing with seed (230,120) and threshold 0.35
    Point seeds[1] = {{230, 120}};
    colorRegionGrowing(inImg, outImg, seeds, 1, 0.35f);

    serial->send(outImg);
  }
  delay(100);
}
