#include "embedDIP.h"
#include <Arduino.h>

const int PIN_BUTTON = 15;

serial_t *serial = &esp32_uart;

Image *inImg    = nullptr;
Image *outImg_1 = nullptr;
Image *outImg_2 = nullptr;

static const uint8_t breakpoints1[] = {0, 128, 255};
static const uint8_t values1[]      = {0,  32, 255};

static const uint8_t breakpoints2[] = {0, 128, 255};
static const uint8_t values2[]      = {0, 200, 255};

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

    piecewiseTransform(inImg, outImg_1, breakpoints1, values1, 3);
    convertTo(outImg_1);
    serial->send(outImg_1);

    piecewiseTransform(inImg, outImg_2, breakpoints2, values2, 3);
    convertTo(outImg_2);
    serial->send(outImg_2);
  }
  delay(100);
}
