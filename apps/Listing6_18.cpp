#include "embedDIP.hpp"
#include <Arduino.h>
#include <vector>

#ifndef PIN_BUTTON
#define PIN_BUTTON 15
#endif

embedDIP::SerialDev serial(&esp32_uart);
SET_LOOP_TASK_STACK_SIZE(16 * 1024 * 2);

embedDIP::Image inImg;
embedDIP::Image outImg;

// Static Sobel-like high-pass filter kernel
static const std::vector<std::vector<float>> highPassFilter3x3 = {
    {1, 0, -1}, {2, 0, -2}, {1, 0, -1}};

void setup() {
  serial.init();

  inImg = embedDIP::Image(IMAGE_RES_QVGA, IMAGE_FORMAT_GRAYSCALE);
  outImg = embedDIP::Image(IMAGE_RES_QVGA, IMAGE_FORMAT_GRAYSCALE);

  pinMode(PIN_BUTTON, INPUT_PULLUP);
}

void loop() {
  if (digitalRead(PIN_BUTTON) == LOW) {
    serial.capture(inImg);

    inImg.filter2D(highPassFilter3x3, outImg);
    outImg.convertTo();
    serial.send(outImg);
  }
  delay(100);
}
