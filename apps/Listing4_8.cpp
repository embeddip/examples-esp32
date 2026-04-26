#include "embedDIP.hpp"
#include "image_data.h"

const int PIN_BUTTON = 15;

SET_LOOP_TASK_STACK_SIZE(16 * 1024 * 2);

embedDIP::SerialDev serial(&esp32_uart);

embedDIP::Image QVGA_RGB565_IMG;

void setup() {
  serial.init();
  QVGA_RGB565_IMG = embedDIP::Image(IMAGE_RES_QVGA, IMAGE_FORMAT_RGB565);
  pinMode(PIN_BUTTON, INPUT_PULLUP);
}

void loop() {
  if (digitalRead(PIN_BUTTON) == LOW) {
    memcpy(QVGA_RGB565_IMG.pixels(), image_data,
           QVGA_RGB565_IMG.size() * QVGA_RGB565_IMG.depth());

    serial.send(QVGA_RGB565_IMG);
  }
  delay(100);
}
