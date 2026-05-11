#include "embedDIP.h"
#include <Arduino.h>

#define PIN_BUTTON 15

SET_LOOP_TASK_STACK_SIZE(16 * 1024 * 2);

serial_t *serial = &esp32_uart;

Image *inImg = nullptr;
Image *outImg = nullptr;
Image *magnitudeImg = nullptr;
Image *phaseImg = nullptr;

void setup() {
  serial->init();
  createImageWH(256, 256, IMAGE_FORMAT_GRAYSCALE, &inImg);
  createImageWH(256, 256, IMAGE_FORMAT_GRAYSCALE, &outImg);
  createImageWH(256, 256, IMAGE_FORMAT_GRAYSCALE, &magnitudeImg);
  createImageWH(256, 256, IMAGE_FORMAT_GRAYSCALE, &phaseImg);
  pinMode(PIN_BUTTON, INPUT_PULLUP);
}

void loop() {
  if (digitalRead(PIN_BUTTON) == LOW) {
    serial->capture(inImg);

    fft(inImg, inImg);
    fftshift(inImg);

    _abs_(inImg, magnitudeImg);
    _phase_(inImg, phaseImg);

    polarToCart(magnitudeImg, phaseImg, outImg);

    fftshift(outImg);
    ifft(outImg, outImg);

    convertTo(outImg);
    serial->send(outImg);
  }
  delay(100);
}
