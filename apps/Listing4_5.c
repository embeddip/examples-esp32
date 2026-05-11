#include "embedDIP.h"

const int PIN_BUTTON = 15;

SET_LOOP_TASK_STACK_SIZE(16 * 1024 * 2);

serial_t *serial = &esp32_uart;

Image *rgbImg = NULL;
Image *grayImg = NULL;
Image *yuvImg = NULL;

void setup() {
  serial->init();
  createImage(IMAGE_RES_QVGA, IMAGE_FORMAT_RGB565, &rgbImg);
  createImage(IMAGE_RES_QVGA, IMAGE_FORMAT_YUV, &yuvImg);
  createImage(IMAGE_RES_QVGA, IMAGE_FORMAT_GRAYSCALE, &grayImg);
  pinMode(PIN_BUTTON, INPUT_PULLUP);
}

void loop() {
  if (digitalRead(PIN_BUTTON) == LOW) {
    serial->capture(rgbImg);
    cvtColor(rgbImg, yuvImg, CVT_RGB565_TO_YUV);
    serial->send(yuvImg);
    cvtColor(yuvImg, grayImg, CVT_YUV_TO_GRAYSCALE);
    serial->send(grayImg);
  }
  delay(100);
}
