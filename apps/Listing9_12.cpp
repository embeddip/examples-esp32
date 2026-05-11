#include <Arduino.h>
#include <embedDIP.hpp>
#include <vector>

#ifndef BUTTON_PIN
#define BUTTON_PIN 15
#endif

SET_LOOP_TASK_STACK_SIZE(16 * 1024 * 2);

embedDIP::SerialDev serial(&esp32_uart);
embedDIP::Camera camera(&esp32_ov2640);

embedDIP::Image previewRgb;
embedDIP::Image gray;
embedDIP::Image denoised;
embedDIP::Image bin;
embedDIP::Image morph;
embedDIP::Image labels;

static int current_mode = 0;

static int lastBtn = HIGH;
static bool pressed = false;
static unsigned long tDown = 0;
static const unsigned long debounceMs = 40;
static const unsigned long longPressMs = 600;
static unsigned long lastChangeMs = 0;

static const std::vector<std::vector<float>> blur3x3 = {
    {1.0f / 9.0f, 1.0f / 9.0f, 1.0f / 9.0f},
    {1.0f / 9.0f, 1.0f / 9.0f, 1.0f / 9.0f},
    {1.0f / 9.0f, 1.0f / 9.0f, 1.0f / 9.0f},
};

static void apply_current_mode() {
  camera.stop();

  previewRgb.cvtColor(gray, CVT_RGB565_TO_GRAYSCALE);
  gray.filter2D(blur3x3, denoised);
  denoised.convertTo();

  embedDIP::Kernel k;
  k.getStructuringElement(MORPH_ELLIPSE, 3);

  switch (current_mode) {
  case 0: // Binary mask
    denoised.grayscaleThreshold(bin, 140);
    bin.cvtColor(morph, CVT_COPY);
    break;

  case 1: // Binary + opening
    denoised.grayscaleThreshold(bin, 140);
    bin.opening(morph, k, 2);
    break;

  case 2: // Binary + closing
    denoised.grayscaleThreshold(bin, 140);
    bin.closing(morph, k, 2);
    break;

  case 3: // Smart inspection
    denoised.grayscaleThreshold(bin, 140);
    bin.opening(morph, k, 2);
    morph.connectedComponents(labels, NULL);
    labels.normalize();
    labels.negative(labels);
    labels.cvtColor(morph, CVT_COPY);
    break;

  default:
    break;
  }

  serial.send(morph);
  camera.capture(CONTINUOUS, previewRgb);
}

void setup() {
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  serial.init();
  camera.init(IMAGE_RES_QVGA, IMAGE_FORMAT_RGB565);

  previewRgb = embedDIP::Image(IMAGE_RES_QVGA, IMAGE_FORMAT_RGB565);
  gray = embedDIP::Image(IMAGE_RES_QVGA, IMAGE_FORMAT_GRAYSCALE);
  denoised = embedDIP::Image(IMAGE_RES_QVGA, IMAGE_FORMAT_GRAYSCALE);
  bin = embedDIP::Image(IMAGE_RES_QVGA, IMAGE_FORMAT_GRAYSCALE);
  morph = embedDIP::Image(IMAGE_RES_QVGA, IMAGE_FORMAT_GRAYSCALE);
  labels = embedDIP::Image(IMAGE_RES_QVGA, IMAGE_FORMAT_GRAYSCALE);
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
        camera.capture(SINGLE, previewRgb);
        apply_current_mode();
      } else {
        current_mode = (current_mode + 1) % 4;
      }
    }
  }

  delay(5);
}
