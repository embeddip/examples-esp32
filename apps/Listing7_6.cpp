#include "embedDIP.hpp"
#include <Arduino.h>

const int PIN_BUTTON = 15;

SET_LOOP_TASK_STACK_SIZE(16 * 1024 * 2);

embedDIP::Image inImg;
embedDIP::Image outImg_1;
embedDIP::Image outImg_2;
embedDIP::SerialDev serial(&esp32_uart);

const std::vector<uint8_t> breakpoints1 = {0, 128, 255};
const std::vector<uint8_t> values1 = {0, 32, 255};

const std::vector<uint8_t> breakpoints2 = {0, 128, 255};
const std::vector<uint8_t> values2 = {0, 200, 255};

void setup() {
  serial.init();

  inImg = embedDIP::Image(IMAGE_RES_QVGA, IMAGE_FORMAT_GRAYSCALE);
  outImg_1 = embedDIP::Image(IMAGE_RES_QVGA, IMAGE_FORMAT_GRAYSCALE);
  outImg_2 = embedDIP::Image(IMAGE_RES_QVGA, IMAGE_FORMAT_GRAYSCALE);

  pinMode(PIN_BUTTON, INPUT_PULLUP);
}

void loop() {
  if (digitalRead(PIN_BUTTON) == LOW) {
    serial.capture(inImg);

    inImg.piecewiseTransform(outImg_1, breakpoints1, values1);
    outImg_1.convertTo();
    serial.send(outImg_1);

    inImg.piecewiseTransform(outImg_2, breakpoints2, values2);
    outImg_2.convertTo();
    serial.send(outImg_2);
  }
  delay(100);
}
