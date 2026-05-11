#include "embedDIP.hpp"
#include <Arduino.h>

#define PIN_BUTTON 15

SET_LOOP_TASK_STACK_SIZE(16 * 1024 * 2);

embedDIP::SerialDev serial(&esp32_uart);

embedDIP::Image inImg;
embedDIP::Image outImg;

embedDIP::Image TabbyCatMagnitude;
embedDIP::Image TabbyCatPhase;

embedDIP::Image KittenMagnitude;
embedDIP::Image KittenPhase;

void setup() {
  serial.init();

  inImg = embedDIP::Image(256, 256, IMAGE_FORMAT_GRAYSCALE);
  outImg = embedDIP::Image(256, 256, IMAGE_FORMAT_GRAYSCALE);

  TabbyCatMagnitude = embedDIP::Image(256, 256, IMAGE_FORMAT_GRAYSCALE);
  TabbyCatPhase = embedDIP::Image(256, 256, IMAGE_FORMAT_GRAYSCALE);

  KittenMagnitude = embedDIP::Image(256, 256, IMAGE_FORMAT_GRAYSCALE);
  KittenPhase = embedDIP::Image(256, 256, IMAGE_FORMAT_GRAYSCALE);

  pinMode(PIN_BUTTON, INPUT_PULLUP);
}

void loop() {
  if (digitalRead(PIN_BUTTON) == LOW) {
    serial.capture(inImg);
    inImg.fft(inImg);
    inImg._abs_(TabbyCatMagnitude);
    inImg._phase_(TabbyCatPhase);

    serial.capture(inImg);
    inImg.fft(inImg);
    inImg._abs_(KittenMagnitude);
    inImg._phase_(KittenPhase);

    outImg.polarToCart(TabbyCatMagnitude, KittenPhase);
    outImg.ifft(outImg);
    outImg.convertTo();
    serial.send(outImg);

    outImg.polarToCart(KittenMagnitude, TabbyCatPhase);
    outImg.ifft(outImg);
    outImg.convertTo();
    serial.send(outImg);
  }
  delay(100);
}
