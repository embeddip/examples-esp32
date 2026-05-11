#include "embedDIP.hpp"

const int PIN_BUTTON = 15;

embedDIP::Image inImg;
embedDIP::Image outImg_1;
embedDIP::Image outImg_2;
embedDIP::SerialDev serial(&esp32_uart);

void setup() {
  serial.init();

  inImg = embedDIP::Image(IMAGE_RES_QVGA, IMAGE_FORMAT_GRAYSCALE);
  outImg_1 = embedDIP::Image(IMAGE_RES_QVGA, IMAGE_FORMAT_GRAYSCALE);
  outImg_2 = embedDIP::Image(IMAGE_RES_QVGA, IMAGE_FORMAT_GRAYSCALE);

  pinMode(PIN_BUTTON, INPUT_PULLUP);
}

void loop() {
  if (digitalRead(PIN_BUTTON) == LOW) {
    serial.capture(inImg);

    inImg.powerTransform(outImg_1, 2.0f);
    outImg_1.convertTo();
    serial.send(outImg_1);

    inImg.powerTransform(outImg_2, 0.5f);
    outImg_2.convertTo();
    serial.send(outImg_2);
  }
  delay(100);
}
