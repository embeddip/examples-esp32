#include "embedDIP.h"
#include <Arduino.h>

const int PIN_BUTTON = 15;
SET_LOOP_TASK_STACK_SIZE(16 * 1024 * 2);

serial_t *serial = &esp32_uart;

Image *inImg = nullptr;

static int histogram[256] = {0};

void setup() {
  serial->init();

  createImage(IMAGE_RES_QVGA, IMAGE_FORMAT_GRAYSCALE, &inImg);

  pinMode(PIN_BUTTON, INPUT_PULLUP);
}

void loop() {
  if (digitalRead(PIN_BUTTON) == LOW) {
    serial->capture(inImg);

    histForm(inImg, histogram);

    serial->send1D(histogram, sizeof(int), 256, SERIAL_DATA_HISTOGRAM);
  }
  delay(100);
}
