#include "embedDIP.hpp"
#include <Arduino.h>

#ifndef PIN_BUTTON
#define PIN_BUTTON 15
#endif

embedDIP::SerialDev serial(&esp32_uart);
SET_LOOP_TASK_STACK_SIZE(16 * 1024 * 2);

embedDIP::Image inImg;
embedDIP::Image outImg;

constexpr uint8_t TARGET_R = 255;
constexpr uint8_t TARGET_G = 0;
constexpr uint8_t TARGET_B = 0;

void setup() {
  serial.init();

  inImg = embedDIP::Image(IMAGE_RES_QVGA, IMAGE_FORMAT_RGB888);
  outImg = embedDIP::Image(IMAGE_RES_QVGA, IMAGE_FORMAT_GRAYSCALE);

  pinMode(PIN_BUTTON, INPUT_PULLUP);
}

void loop() {
  if (digitalRead(PIN_BUTTON) == LOW) {
    serial.capture(inImg);

    inImg.dist(outImg, TARGET_R, TARGET_G, TARGET_B);

    outImg.convertTo();
    serial.send(outImg);
  }
  delay(100);
}
