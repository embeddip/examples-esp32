#include <Arduino.h>
#include <embedDIP.h>

#ifndef BUTTON_PIN
#define BUTTON_PIN 15
#endif

serial_t *serial = &esp32_uart;
camera_t *camera = &esp32_ov2640;

Image *inImg = nullptr;
Image *filtered = nullptr;
Image *outImg = nullptr;

static int current_mode = 0;

static int lastBtn = HIGH;
static bool pressed = false;
static unsigned long tDown = 0;
static const unsigned long debounceMs = 40;
static const unsigned long longPressMs = 600;
static unsigned long lastChangeMs = 0;

static const float highPassFilter3x3[9] = {
    0, -1, 0,
    -1, 4, -1,
    0, -1, 0};

static void apply_current_mode() {
  camera->stop();

  switch (current_mode) {
  case 0:
    histEq(inImg, filtered);
    medianFilter(filtered, outImg, 3);
    normalize(outImg);
    convertTo(outImg);
    break;

  case 1:
    negative(inImg, outImg);
    break;

  case 2:
    powerTransform(inImg, outImg, 1.5f / 1.0f);
    normalize(outImg);
    convertTo(outImg);
    break;

  case 3:
    filter2D(inImg, filtered, highPassFilter3x3, 3);
    add(inImg, filtered, outImg);
    convertTo(outImg);
    break;

  default:
    break;
  }

  serial->send(outImg);
  camera->capture(CONTINUOUS, inImg);
}

void setup() {
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  serial->init();
  camera->init(IMAGE_RES_QVGA, IMAGE_FORMAT_GRAYSCALE);

  createImage(IMAGE_RES_QVGA, IMAGE_FORMAT_GRAYSCALE, &inImg);
  createImage(IMAGE_RES_QVGA, IMAGE_FORMAT_GRAYSCALE, &filtered);
  createImage(IMAGE_RES_QVGA, IMAGE_FORMAT_GRAYSCALE, &outImg);

  camera->capture(CONTINUOUS, inImg);
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
