#include "embedDIP.hpp"
#include <Arduino.h>

const int PIN_BUTTON = 15;

SET_LOOP_TASK_STACK_SIZE(16 * 1024 * 2);

embedDIP::Image inImg;
embedDIP::SerialDev serial(&esp32_uart);

void setup() {
  serial.init();

  inImg = embedDIP::Image(IMAGE_RES_QVGA, IMAGE_FORMAT_GRAYSCALE);
  pinMode(PIN_BUTTON, INPUT_PULLUP);
}

void loop() {
  if (digitalRead(PIN_BUTTON) == LOW) {
    serial.capture(inImg);

    std::vector<int> histogram(256, 0);
    inImg.histForm(histogram);

    serial.send1D(histogram.data(), sizeof(int), histogram.size(),
                  SERIAL_DATA_HISTOGRAM);
  }
  delay(100);
}
