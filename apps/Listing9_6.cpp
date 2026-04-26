#include "embedDIP.hpp"
#include <Arduino.h>

#ifndef PIN_BUTTON
#define PIN_BUTTON 15
#endif

SET_LOOP_TASK_STACK_SIZE(16 * 1024 * 2);

embedDIP::SerialDev serial(&esp32_uart);

// Image buffers
embedDIP::Image inImg;        // RGB888 input
embedDIP::Image hsiImg;       // Converted to HSI
embedDIP::Image hsiREDImg;    // Red hue mask
embedDIP::Image hsiYELLOWImg; // Yellow hue mask
embedDIP::Image hsiMergeImg;  // Combined mask
embedDIP::Image outImg;       // Final RGB output

void setup() {
  serial.init();

  inImg = embedDIP::Image(IMAGE_RES_QVGA, IMAGE_FORMAT_RGB888);
  hsiImg = embedDIP::Image(IMAGE_RES_QVGA, IMAGE_FORMAT_HSI);
  hsiREDImg = embedDIP::Image(IMAGE_RES_QVGA, IMAGE_FORMAT_GRAYSCALE);
  hsiYELLOWImg = embedDIP::Image(IMAGE_RES_QVGA, IMAGE_FORMAT_GRAYSCALE);
  hsiMergeImg = embedDIP::Image(IMAGE_RES_QVGA, IMAGE_FORMAT_GRAYSCALE);
  outImg = embedDIP::Image(IMAGE_RES_QVGA, IMAGE_FORMAT_RGB888);

  pinMode(PIN_BUTTON, INPUT_PULLUP);
}

void loop() {
  if (digitalRead(PIN_BUTTON) == LOW) {
    serial.capture(inImg);

    inImg.cvtColor(hsiImg, CVT_RGB888_TO_HSI);

    hsiImg.hueThreshold(hsiREDImg, 241, 14);
    hsiImg.hueThreshold(hsiYELLOWImg, 28, 42);

    hsiREDImg._or(hsiYELLOWImg, hsiMergeImg);

    inImg._and_(hsiMergeImg, outImg);

    serial.send(outImg);
  }
  delay(100);
}
