#include "embedDIP.hpp"

const int PIN_BUTTON = 15;

SET_LOOP_TASK_STACK_SIZE(16 * 1024 * 2);

embedDIP::SerialDev serial(&esp32_uart);
embedDIP::Camera camera(&esp32_ov2640);

embedDIP::Image inImg;
embedDIP::Image compressedImg;

void setup() {
  serial.init();
  camera.init(IMAGE_RES_QQVGA, IMAGE_FORMAT_RGB565);

  inImg = embedDIP::Image(IMAGE_RES_QQVGA, IMAGE_FORMAT_RGB565);
  compressedImg = embedDIP::Image(IMAGE_RES_QQVGA, IMAGE_FORMAT_RGB888);

  pinMode(PIN_BUTTON, INPUT_PULLUP);
}

void loop() {
  if (digitalRead(PIN_BUTTON) == LOW) {
    while (digitalRead(PIN_BUTTON) == LOW) {
      if (camera.capture(SINGLE, inImg) &&
          inImg.compressJPEG(compressedImg, 75) == 0) {
        serial.sendJPEG(compressedImg);
      }
    }
  }
  delay(100);
}
