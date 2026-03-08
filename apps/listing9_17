#include "embedDIP.h"
#include <Arduino.h>

#ifndef PIN_BUTTON
#define PIN_BUTTON 15
#endif

SET_LOOP_TASK_STACK_SIZE(16 * 1024 * 2);

serial_t *serial = &esp32_uart;

Image *inImg  = nullptr;
Image *gX     = nullptr;
Image *gY     = nullptr;
Image *outImg = nullptr;

void setup() {
  serial->init();
  createImage(IMAGE_RES_QVGA, IMAGE_FORMAT_GRAYSCALE, &inImg);
  createImage(IMAGE_RES_QVGA, IMAGE_FORMAT_GRAYSCALE, &gX);
  createImage(IMAGE_RES_QVGA, IMAGE_FORMAT_GRAYSCALE, &gY);
  createImage(IMAGE_RES_QVGA, IMAGE_FORMAT_GRAYSCALE, &outImg);
  pinMode(PIN_BUTTON, INPUT_PULLUP);
}

void loop() {
  if (digitalRead(PIN_BUTTON) == LOW) {
    // Capture input
    serial->capture(inImg);

    // Gaussian gradients
    gaussianGradients(inImg, gX, gY, 1.2f);

    // Send gx, gy
    convertTo(gX);
    serial->send(gX);

    convertTo(gY);
    serial->send(gY);

    // Gradient magnitude from gx, gy
    gradientMagnitude(gX, gY, outImg);
    convertTo(outImg);
    serial->send(outImg);
  }
  delay(100);
}
