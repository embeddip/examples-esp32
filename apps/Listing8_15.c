#include "embedDIP.h"
#include <Arduino.h>

#define PIN_BUTTON 15

SET_LOOP_TASK_STACK_SIZE(16 * 1024 * 2);

serial_t *serial = &esp32_uart;

Image *inImg = nullptr;
Image *fftImg = nullptr;
Image *filter = nullptr;
Image *outImg = nullptr;

void setup() {
  serial->init();
  createImageWH(256, 256, IMAGE_FORMAT_GRAYSCALE, &inImg);
  createImageWH(256, 256, IMAGE_FORMAT_GRAYSCALE, &fftImg);
  createImageWH(256, 256, IMAGE_FORMAT_GRAYSCALE, &filter);
  createImageWH(256, 256, IMAGE_FORMAT_GRAYSCALE, &outImg);
  pinMode(PIN_BUTTON, INPUT_PULLUP);
}

void loop() {
  if (digitalRead(PIN_BUTTON) == LOW) {
    serial->capture(inImg);

    fft(inImg, fftImg);
    fftshift(fftImg);

    getFilter(filter, FREQ_FILTER_IDEAL_HIGHPASS, 30.0f, 0.0f);

    ffilter2D(fftImg, filter, outImg);

    fftshift(outImg);
    ifft(outImg, outImg);

    convertTo(outImg);
    serial->send(outImg);
  }
  delay(100);
}
