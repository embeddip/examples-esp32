#include "embedDIP.hpp"
#include <Arduino.h>

const int PIN_BUTTON = 15;
int button_state = 0;

SET_LOOP_TASK_STACK_SIZE(16 * 1024 * 2);

embedDIP::Image srcImg;
embedDIP::Image refImg;
embedDIP::Image outImg;
embedDIP::SerialDev serial(&esp32_uart);

void setup() {
  serial.init();

  srcImg = embedDIP::Image(IMAGE_RES_QVGA, IMAGE_FORMAT_GRAYSCALE);
  refImg = embedDIP::Image(IMAGE_RES_QVGA, IMAGE_FORMAT_GRAYSCALE);
  outImg = embedDIP::Image(IMAGE_RES_QVGA, IMAGE_FORMAT_GRAYSCALE);

  pinMode(PIN_BUTTON, INPUT_PULLUP);
}

void loop() {
  if (digitalRead(PIN_BUTTON) == LOW) {
    for (int img_idx = 0; img_idx < 4; ++img_idx) {
      serial.capture(refImg);
      std::vector<int> histogram(256, 0);
      refImg.histForm(histogram);

      serial.capture(srcImg);

      srcImg.histSpec(outImg, histogram);

      outImg.convertTo();
      serial.send(outImg);
    }
  }
  delay(100);
}
