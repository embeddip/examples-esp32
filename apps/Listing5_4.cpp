#include "embedDIP.h"

#define PIN_BUTTON 15

SET_LOOP_TASK_STACK_SIZE(16 * 1024 * 2);

embedDIP::Image inImg;
embedDIP::Camera camera(&esp32_ov2640);

void setup() {
  inImg = embedDIP::Image(IMAGE_RES_QVGA, IMAGE_FORMAT_RGB888);
  pinMode(PIN_BUTTON, INPUT_PULLUP);
}

void loop() {
  if (digitalRead(PIN_BUTTON) == LOW) {
    camera.init(IMAGE_RES_QVGA, IMAGE_FORMAT_RGB888);
    camera.capture(SINGLE, inImg);
  }
  delay(100);
}
