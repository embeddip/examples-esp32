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
  camera->init(IMAGE_RES_QVGA, IMAGE_FORMAT_RGB565);
  createImage(IMAGE_RES_QVGA, IMAGE_FORMAT_RGB565, &qvgaImg);
  createImage(IMAGE_RES_QQVGA, IMAGE_FORMAT_RGB565, &qqvgaImg);
  pinMode(PIN_BUTTON, INPUT_PULLUP);
}

void loop() {
  if (digitalRead(PIN_BUTTON) == LOW) {
    if (camera->capture(SINGLE, qvgaImg) == 0) {
      serial->send(qvgaImg);
    }

    if (camera->setRes(IMAGE_RES_QQVGA) == 0) {
      // First frame after framesize switch may be unstable; discard once.
      (void)camera->capture(SINGLE, qqvgaImg);
      if (camera->capture(SINGLE, qqvgaImg) == 0) {
        serial->send(qqvgaImg);
      }
    }

    (void)camera->setRes(IMAGE_RES_QVGA);
    (void)camera->capture(SINGLE, qvgaImg);

    while (digitalRead(PIN_BUTTON) == LOW) {
      delay(5);
    }
  }
  delay(100);
}
