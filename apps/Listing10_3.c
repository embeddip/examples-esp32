#include "embedDIP.h"
#include <Arduino.h>

#ifndef PIN_BUTTON
#define PIN_BUTTON 15
#endif

SET_LOOP_TASK_STACK_SIZE(16 * 1024 * 2);

serial_t *serial = &esp32_uart;

Image *inImg = NULL;
Image *outImg = NULL;

static Kernel k;

void setup() {
  createImage(IMAGE_RES_QVGA, IMAGE_FORMAT_GRAYSCALE, &inImg);
  createImage(IMAGE_RES_QVGA, IMAGE_FORMAT_GRAYSCALE, &outImg);
  serial->init();

  getStructuringElement(&k, MORPH_ELLIPSE, 3);

  pinMode(PIN_BUTTON, INPUT_PULLUP);
}

void loop() {
  if (digitalRead(PIN_BUTTON) == LOW) {
    // Capture and binarize
    serial->capture(inImg);
    grayscaleThreshold(inImg, inImg, 128);

    // Erode with 1 iteration
    erode(inImg, outImg, &k, 1);
    serial->send(outImg);

    // Erode with 2 iterations
    erode(inImg, outImg, &k, 2);
    serial->send(outImg);

    // Erode with 3 iterations
    erode(inImg, outImg, &k, 3);
    serial->send(outImg);
  }
  delay(100);
}
