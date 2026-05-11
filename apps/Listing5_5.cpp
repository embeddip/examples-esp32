#include <Arduino.h>
#include <embedDIP.hpp>

#define PIN_BUTTON 15

SET_LOOP_TASK_STACK_SIZE(16 * 1024 * 2);

embedDIP::Image qvgaImg;
embedDIP::Image qqvgaImg;

embedDIP::SerialDev serial(&esp32_uart);
embedDIP::Camera camera(&esp32_ov2640);

void setup() {
  serial.init();
  qvgaImg = embedDIP::Image(IMAGE_RES_QVGA, IMAGE_FORMAT_RGB565);
  qqvgaImg = embedDIP::Image(IMAGE_RES_QQVGA, IMAGE_FORMAT_RGB565);
  pinMode(PIN_BUTTON, INPUT_PULLUP);
}

void loop() {
  if (digitalRead(PIN_BUTTON) == LOW) {
    camera.init(IMAGE_RES_QVGA, IMAGE_FORMAT_RGB565);
    camera.capture(SINGLE, qvgaImg);
    serial.send(qvgaImg);
    camera.stop();

    camera.setRes(IMAGE_RES_QQVGA);
    camera.capture(SINGLE, qqvgaImg);
    serial.send(qqvgaImg);
  }
  delay(100);
}
