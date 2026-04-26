#include "embedDIP.h"
#include <Arduino.h>

const int PIN_BUTTON = 15;

serial_t *serial = &esp32_uart;

Image *inImg    = nullptr;
Image *outImg_1 = nullptr;
Image *outImg_2 = nullptr;

void setup() {
  serial->init();

  createImage(IMAGE_RES_QVGA, IMAGE_FORMAT_GRAYSCALE, &inImg);
  createImage(IMAGE_RES_QVGA, IMAGE_FORMAT_GRAYSCALE, &outImg_1);
  createImage(IMAGE_RES_QVGA, IMAGE_FORMAT_GRAYSCALE, &outImg_2);

  pinMode(PIN_BUTTON, INPUT_PULLUP);
}

void loop() {
  if (digitalRead(PIN_BUTTON) == LOW) {
    serial->capture(inImg);

    powerTransform(inImg, outImg_1, 2.0f);
    convertTo(outImg_1);
    serial->send(outImg_1);

    powerTransform(inImg, outImg_2, 1.0f / 2.0f);
    convertTo(outImg_2);
    serial->send(outImg_2);
  }
  delay(100);
}
