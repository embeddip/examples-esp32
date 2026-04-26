#include "embedDIP.hpp"
#include <Arduino.h>

#ifndef PIN_BUTTON
#define PIN_BUTTON 15
#endif

SET_LOOP_TASK_STACK_SIZE(16 * 1024 * 2);

embedDIP::SerialDev serial(&esp32_uart);

embedDIP::Image inImg;
embedDIP::Image outImg;

embedDIP::Kernel k;

void setup() {
  serial.init();

  inImg = embedDIP::Image(IMAGE_RES_QVGA, IMAGE_FORMAT_GRAYSCALE);
  outImg = embedDIP::Image(IMAGE_RES_QVGA, IMAGE_FORMAT_GRAYSCALE);

  // Prepare 3x3 elliptical SE once
  k.getStructuringElement(MORPH_ELLIPSE, 3);

  pinMode(PIN_BUTTON, INPUT_PULLUP);
}

void loop() {
  if (digitalRead(PIN_BUTTON) == LOW) {
    // 1) Capture input
    serial.capture(inImg);

    // 2) Threshold in-place
    inImg.grayscaleThreshold(inImg, 128);

    // 3) Erode with the ellipse SE, 3 iterations
    inImg.erode(outImg, k, 3);

    // 4) Send result
    outImg.convertTo();
    serial.send(outImg);
  }

  delay(100);
}
