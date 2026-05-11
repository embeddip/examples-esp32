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
embedDIP::Image rImg;
embedDIP::Image gImg;
embedDIP::Image bImg;
embedDIP::Image rOutImg;
embedDIP::Image gOutImg;
embedDIP::Image bOutImg;

// Static 3x3 box low-pass filter
static const std::vector<std::vector<float>> lowPassFilter3x3 = {
    {1.0f / 9.0f, 1.0f / 9.0f, 1.0f / 9.0f},
    {1.0f / 9.0f, 1.0f / 9.0f, 1.0f / 9.0f},
    {1.0f / 9.0f, 1.0f / 9.0f, 1.0f / 9.0f}};

void setup() {
  serial.init();

  inImg = embedDIP::Image(IMAGE_RES_QVGA, IMAGE_FORMAT_RGB888);
  outImg = embedDIP::Image(IMAGE_RES_QVGA, IMAGE_FORMAT_RGB888);

  rImg = embedDIP::Image(IMAGE_RES_QVGA, IMAGE_FORMAT_GRAYSCALE);
  gImg = embedDIP::Image(IMAGE_RES_QVGA, IMAGE_FORMAT_GRAYSCALE);
  bImg = embedDIP::Image(IMAGE_RES_QVGA, IMAGE_FORMAT_GRAYSCALE);

  rOutImg = embedDIP::Image(IMAGE_RES_QVGA, IMAGE_FORMAT_GRAYSCALE);
  gOutImg = embedDIP::Image(IMAGE_RES_QVGA, IMAGE_FORMAT_GRAYSCALE);
  bOutImg = embedDIP::Image(IMAGE_RES_QVGA, IMAGE_FORMAT_GRAYSCALE);

  pinMode(PIN_BUTTON, INPUT_PULLUP);
}

void loop() {
  if (digitalRead(PIN_BUTTON) == LOW) {
    serial.capture(inImg);

    inImg.rgbSplit(rImg, gImg, bImg);

    rImg.filter2D(lowPassFilter3x3, rOutImg);
    gImg.filter2D(lowPassFilter3x3, gOutImg);
    bImg.filter2D(lowPassFilter3x3, bOutImg);

    rOutImg.convertTo();
    gOutImg.convertTo();
    bOutImg.convertTo();

    outImg.rgbMerge(rOutImg, gOutImg, bOutImg);

    serial.send(outImg);
  }
  delay(100);
}
