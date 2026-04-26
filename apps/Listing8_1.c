#include <Arduino.h>
#include <embedDIP.h>

#define PIN_BUTTON 15

SET_LOOP_TASK_STACK_SIZE(16 * 1024 * 2);

serial_t *serial = &esp32_uart;

Image *inImg = nullptr;
Image *fftImg = nullptr;
Image *magnitudeImg = nullptr;
Image *phaseImg = nullptr;

void setup() {
  pinMode(PIN_BUTTON, INPUT_PULLUP);
  serial->init();

  createImageWH(256, 256, IMAGE_FORMAT_GRAYSCALE, &inImg);
  createImageWH(256, 256, IMAGE_FORMAT_GRAYSCALE, &fftImg);
  createImageWH(256, 256, IMAGE_FORMAT_GRAYSCALE, &magnitudeImg);
  createImageWH(256, 256, IMAGE_FORMAT_GRAYSCALE, &phaseImg);
}

void loop() {
  if (digitalRead(PIN_BUTTON) == LOW) {
    serial->capture(inImg);

    fft(inImg, fftImg);
    fftshift(fftImg);

    _abs_(fftImg, magnitudeImg);
    _add_(magnitudeImg, 1.0f);
    _log_(magnitudeImg);
    convertTo(magnitudeImg);

    _phase_(fftImg, phaseImg);
    convertTo(phaseImg);

    serial->send(magnitudeImg);
    serial->send(phaseImg);
  }
  delay(100);
}
