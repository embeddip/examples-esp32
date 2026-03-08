#include <Arduino.h>
#include <embedDIP.h>

#define PIN_BUTTON 15

SET_LOOP_TASK_STACK_SIZE(16 * 1024 * 2);

serial_t *serial = &esp32_uart;
camera_t *camera = &esp32_ov2640;

Image *qvgaImg = nullptr;

void setup() {
  createImage(IMAGE_RES_QVGA, IMAGE_FORMAT_RGB565, &qvgaImg);
  pinMode(PIN_BUTTON, INPUT_PULLUP);
  serial->init();
}

void loop() {
  if (digitalRead(PIN_BUTTON) == LOW) {
    camera->init(IMAGE_RES_QVGA, IMAGE_FORMAT_RGB565);
    camera->capture(SINGLE, qvgaImg);
    serial->send(qvgaImg);
  }
  delay(100);
}
