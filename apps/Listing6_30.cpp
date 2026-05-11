#include "embedDIP.hpp"
#include <Arduino.h>
#include <vector>

#ifndef BUTTON_PIN
#define BUTTON_PIN 15
#endif

embedDIP::SerialDev serial(&esp32_uart);
embedDIP::Camera camera(&esp32_ov2640);
SET_LOOP_TASK_STACK_SIZE(16 * 1024 * 2);

embedDIP::Image inImg;
embedDIP::Image filtered;
embedDIP::Image outImg;

static int current_mode = 0;

static int lastBtn = HIGH;
static bool pressed = false;
static unsigned long tDown = 0;
static const unsigned long debounceMs = 40;
static const unsigned long longPressMs = 600;
static unsigned long lastChangeMs = 0;

static void apply_current_mode() {
  camera.stop();

  switch (current_mode) {
  case 0:
    inImg.histEq(filtered);
    filtered.medianFilter(outImg, 3);
    outImg.convertTo();
    break;

  case 1:
    inImg.negative(outImg);
    break;

  case 2:
    inImg.powerTransform(outImg, 1.5f);
    outImg.convertTo();
    break;

  case 3: {
    static const std::vector<std::vector<float>> highPassFilter3x3 = {
        {0, -1, 0}, {-1, 4, -1}, {0, -1, 0}};
    inImg.filter2D(highPassFilter3x3, filtered);
    inImg.add(filtered, outImg);
    outImg.convertTo();
    break;
  }

  default:
    break;
  }

  serial.send(outImg);
  camera.capture(CONTINUOUS, inImg);
}

void setup() {
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  serial.init();
  camera.init(IMAGE_RES_QVGA, IMAGE_FORMAT_GRAYSCALE);

  inImg = embedDIP::Image(IMAGE_RES_QVGA, IMAGE_FORMAT_GRAYSCALE);
  filtered = embedDIP::Image(IMAGE_RES_QVGA, IMAGE_FORMAT_GRAYSCALE);
  outImg = embedDIP::Image(IMAGE_RES_QVGA, IMAGE_FORMAT_GRAYSCALE);

  camera.capture(CONTINUOUS, inImg);
}

void loop() {
  int reading = digitalRead(BUTTON_PIN);
  if (reading != lastBtn) {
    lastChangeMs = millis();
    lastBtn = reading;
  }

  if ((millis() - lastChangeMs) > debounceMs) {
    if (!pressed && reading == LOW) {
      pressed = true;
      tDown = millis();
    }
    if (pressed && reading == HIGH) {
      unsigned long held = millis() - tDown;
      pressed = false;

      if (held >= longPressMs) {
        apply_current_mode();
      } else {
        current_mode = (current_mode + 1) % 4;
      }
    }
  }

  delay(5);
}
