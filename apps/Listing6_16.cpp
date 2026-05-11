#include "embedDIP.hpp"
#include <Arduino.h>
#include <vector>

#ifndef PIN_BUTTON
#define PIN_BUTTON 15
#endif

SET_LOOP_TASK_STACK_SIZE(16 * 1024 * 2);

embedDIP::SerialDev serial(&esp32_uart);
embedDIP::Image inImg;
embedDIP::Image outImg;

std::vector<std::vector<float>> lp3(3, std::vector<float>(3, 1.0f / 9.0f));
std::vector<std::vector<float>> lp5(5, std::vector<float>(5, 1.0f / 25.0f));

void setup() {
  serial.init();

  inImg = embedDIP::Image(IMAGE_RES_QVGA, IMAGE_FORMAT_GRAYSCALE);
  outImg = embedDIP::Image(IMAGE_RES_QVGA, IMAGE_FORMAT_GRAYSCALE);

  pinMode(PIN_BUTTON, INPUT_PULLUP);
}

void loop() {
  if (digitalRead(PIN_BUTTON) == LOW) {
    serial.capture(inImg);

    inImg.filter2D(lp3, outImg);
    outImg.convertTo();
    serial.send(outImg);

    inImg.filter2D(lp5, outImg);
    outImg.convertTo();
    serial.send(outImg);
  }
  delay(100);
}
