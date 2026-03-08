#include "embedDIP.h"

#define PIN_BUTTON 15

Image *inImg = nullptr;
camera_t *camera = &esp32_ov2640;

void setup() {
  createImage(IMAGE_RES_QVGA, IMAGE_FORMAT_RGB888, &inImg);
  pinMode(PIN_BUTTON, INPUT_PULLUP);
}

void loop() {
  if (digitalRead(PIN_BUTTON) == LOW) {
    camera->init(IMAGE_RES_QVGA, IMAGE_FORMAT_RGB888);
    camera->capture(SINGLE, inImg);
  }
  delay(100);
}
