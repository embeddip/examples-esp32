#include "embedDIP.hpp"
#include <Arduino.h>

#define PIN_BUTTON 15

SET_LOOP_TASK_STACK_SIZE(16 * 1024 * 2);

embedDIP::SerialDev serial(&esp32_uart);

embedDIP::Image inImg;
embedDIP::Image outImg;
embedDIP::Image magnitudeImg;
embedDIP::Image phaseImg;

void setup() {
  serial.init();

  inImg = embedDIP::Image(256, 256, IMAGE_FORMAT_GRAYSCALE);
  outImg = embedDIP::Image(256, 256, IMAGE_FORMAT_GRAYSCALE);
  magnitudeImg = embedDIP::Image(256, 256, IMAGE_FORMAT_GRAYSCALE);
  phaseImg = embedDIP::Image(256, 256, IMAGE_FORMAT_GRAYSCALE);

  pinMode(PIN_BUTTON, INPUT_PULLUP);
}

void loop() {
  if (digitalRead(PIN_BUTTON) == LOW) {
    serial.capture(inImg);

    inImg.fft(inImg);
    inImg.fftshift();

    inImg._abs_(magnitudeImg);
    inImg._phase_(phaseImg);

    outImg.polarToCart(magnitudeImg, phaseImg);
    outImg.fftshift();
    outImg.ifft(outImg);

    outImg.convertTo();
    serial.send(outImg);
  }
  delay(100);
}
