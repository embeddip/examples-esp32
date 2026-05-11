#include "embedDIP.hpp"
#include <Arduino.h>
#include <vector>

#ifndef PIN_BUTTON
#define PIN_BUTTON 15
#endif

SET_LOOP_TASK_STACK_SIZE(16 * 1024 * 2);

embedDIP::SerialDev serial(&esp32_uart);

embedDIP::Image inImg;
embedDIP::Image sX;
embedDIP::Image sY;
embedDIP::Image outImg;

static const std::vector<std::vector<float>> SOBEL_X_3x3 = {
    {-1.f, 0.f, 1.f}, {-2.f, 0.f, 2.f}, {-1.f, 0.f, 1.f}};

static const std::vector<std::vector<float>> SOBEL_Y_3x3 = {
    {1.f, 2.f, 1.f}, {0.f, 0.f, 0.f}, {-1.f, -2.f, -1.f}};

void setup() {
  serial.init();

  inImg = embedDIP::Image(IMAGE_RES_QVGA, IMAGE_FORMAT_GRAYSCALE);
  sX = embedDIP::Image(IMAGE_RES_QVGA, IMAGE_FORMAT_GRAYSCALE);
  sY = embedDIP::Image(IMAGE_RES_QVGA, IMAGE_FORMAT_GRAYSCALE);
  outImg = embedDIP::Image(IMAGE_RES_QVGA, IMAGE_FORMAT_GRAYSCALE);

  pinMode(PIN_BUTTON, INPUT_PULLUP);
}

void loop() {
  if (digitalRead(PIN_BUTTON) == LOW) {
    serial.capture(inImg);

    inImg.filter2D(SOBEL_X_3x3, sX);
    inImg.filter2D(SOBEL_Y_3x3, sY);

    sX.convertTo();
    serial.send(sX);

    sY.convertTo();
    serial.send(sY);

    outImg.gradientMagnitude(sX, sY);

    outImg.convertTo();
    serial.send(outImg);
  }

  delay(100);
}
