#include "embedDIP.hpp"
#include <Arduino.h>

#ifndef PIN_BUTTON
#define PIN_BUTTON 15
#endif

SET_LOOP_TASK_STACK_SIZE(16 * 1024 * 2);

embedDIP::SerialDev serial(&esp32_uart);

embedDIP::Image inImg;
embedDIP::Image outImg;

void setup() {
  serial.init();

  inImg = embedDIP::Image(IMAGE_RES_QVGA, IMAGE_FORMAT_RGB888);
  outImg = embedDIP::Image(IMAGE_RES_QVGA, IMAGE_FORMAT_RGB888);

  pinMode(PIN_BUTTON, INPUT_PULLUP);
}

void loop() {
  if (digitalRead(PIN_BUTTON) == LOW) {
    serial.capture(inImg);

    embedDIP::Point seeds[1] = {{230, 120}};
    inImg.colorRegionGrowing(outImg, seeds, 1, 0.35f);

    serial.send(outImg);
  }

  delay(100);
}
