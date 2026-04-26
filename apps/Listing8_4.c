#include "embedDIP.h"
#include <Arduino.h>

#define PIN_BUTTON 15

SET_LOOP_TASK_STACK_SIZE(16 * 1024 * 2);

serial_t *serial = &esp32_uart;

Image *inImg = nullptr;
Image *outImg = nullptr;

Image *KittenMagnitude = nullptr;
Image *KittenPhase = nullptr;

Image *TabbyCatMagnitude = nullptr;
Image *TabbyCatPhase = nullptr;

void setup() {
  serial->init();

  createImageWH(256, 256, IMAGE_FORMAT_GRAYSCALE, &inImg);
  createImageWH(256, 256, IMAGE_FORMAT_GRAYSCALE, &outImg);
  createImageWH(256, 256, IMAGE_FORMAT_GRAYSCALE, &KittenMagnitude);
  createImageWH(256, 256, IMAGE_FORMAT_GRAYSCALE, &KittenPhase);
  createImageWH(256, 256, IMAGE_FORMAT_GRAYSCALE, &TabbyCatMagnitude);
  createImageWH(256, 256, IMAGE_FORMAT_GRAYSCALE, &TabbyCatPhase);

  pinMode(PIN_BUTTON, INPUT_PULLUP);
}

void loop() {
  if (digitalRead(PIN_BUTTON) == LOW) {

    serial->capture(inImg);
    fft(inImg, inImg);
    _abs_(inImg, KittenMagnitude);
    _phase_(inImg, KittenPhase);

    serial->capture(inImg);
    fft(inImg, inImg);
    _abs_(inImg, TabbyCatMagnitude);
    _phase_(inImg, TabbyCatPhase);

    polarToCart(TabbyCatMagnitude, KittenPhase, outImg);
    ifft(outImg, outImg);
    convertTo(outImg);
    serial->send(outImg);

    polarToCart(KittenMagnitude, TabbyCatPhase, outImg);
    ifft(outImg, outImg);
    convertTo(outImg);
    serial->send(outImg);
  }
  delay(100);
}
