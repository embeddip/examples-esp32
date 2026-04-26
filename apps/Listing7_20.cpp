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

// Static 1D low-pass kernel [1, 1, 1]
static const std::vector<float> LP3 = {1.0f, 1.0f, 1.0f};
constexpr float NORM_3x3 = 1.0f / 9.0f;

void setup() {
  serial.init();

  inImg = embedDIP::Image(IMAGE_RES_QVGA, IMAGE_FORMAT_GRAYSCALE);
  outImg = embedDIP::Image(IMAGE_RES_QVGA, IMAGE_FORMAT_GRAYSCALE);

  pinMode(PIN_BUTTON, INPUT_PULLUP);
}

void loop() {
  if (digitalRead(PIN_BUTTON) == LOW) {
    serial.capture(inImg);

    // Separable 3x3 box blur = LP3 (rows) + LP3 (cols) with 1/9 normalization
    inImg.sepFilter2D(outImg, LP3, LP3, NORM_3x3);

    outImg.convertTo();
    serial.send(outImg);
  }
  delay(100);
}
