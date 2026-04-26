#include <Arduino.h>
#include <embedDIP.h>

#define PIN_BUTTON 15
int button_state = 0;

SET_LOOP_TASK_STACK_SIZE(16 * 1024 * 2);

Image *qvgaImg = nullptr;
Image *qqvgaImg = nullptr;

serial_t *serial = &esp32_uart;
camera_t *camera = &esp32_ov2640;

void setup() {
  serial->init();
  createImage(IMAGE_RES_QVGA, IMAGE_FORMAT_RGB565, &qvgaImg);
  createImage(IMAGE_RES_QQVGA, IMAGE_FORMAT_RGB565, &qqvgaImg);
  pinMode(PIN_BUTTON, INPUT_PULLUP);
}

void loop() {
  if (digitalRead(PIN_BUTTON) == LOW) {
    camera->init(IMAGE_RES_QVGA, IMAGE_FORMAT_RGB565);
    camera->capture(SINGLE, qvgaImg);
    serial->send(qvgaImg);
    camera->stop();
    
    camera->setRes(IMAGE_RES_QQVGA);
    camera->capture(SINGLE, qqvgaImg);
    serial->send(qqvgaImg);
  }
  delay(100);
}
