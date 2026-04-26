#include <Arduino.h>
#include <embedDIP.h>

#define PIN_BUTTON 15

SET_LOOP_TASK_STACK_SIZE(16 * 1024 * 2);

serial_t *serial = &esp32_uart;
camera_t *camera = &esp32_ov2640;

Image *inImg = nullptr;
Image *compressedImg = nullptr;

void setup() {
  serial->init();
  camera->init(IMAGE_RES_QQVGA, IMAGE_FORMAT_RGB565);

  createImage(IMAGE_RES_QQVGA, IMAGE_FORMAT_RGB565, &inImg);
  createImage(IMAGE_RES_QQVGA, IMAGE_FORMAT_RGB888, &compressedImg);

  pinMode(PIN_BUTTON, INPUT_PULLUP);
}

void loop() {
  if (digitalRead(PIN_BUTTON) == LOW) {
    while (digitalRead(PIN_BUTTON) == LOW) {
      if (camera->capture(SINGLE, inImg) &&
          compress(inImg, compressedImg, IMAGE_COMP_JPEG, 75) == 0) {
        serial->sendJPEG(compressedImg);
      }
    }
  }
  delay(100);
}
