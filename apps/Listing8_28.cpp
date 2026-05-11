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
embedDIP::Image edges;
embedDIP::Image clean;
embedDIP::Image gradX;
embedDIP::Image gradY;
embedDIP::Image mag;

static int current_mode = 0;

static int lastBtn = HIGH;
static bool pressed = false;
static unsigned long tDown = 0;
static const unsigned long debounceMs = 40;
static const unsigned long longPressMs = 600;
static unsigned long lastChangeMs = 0;

static const std::vector<std::vector<float>> kernelX = {
    {-1, 0, 1},
    {-2, 0, 2},
    {-1, 0, 1},
};

static const std::vector<std::vector<float>> kernelY = {
    {1, 2, 1},
    {0, 0, 0},
    {-1, -2, -1},
};

static void apply_current_mode() {
  camera.stop();
  previewRgb.cvtColor(gray, CVT_RGB565_TO_GRAYSCALE);

  embedDIP::Image *result = nullptr;

  switch (current_mode) {
  case 0: // LoG-based edge mask.
    gray.logFilter(edges, 1.4f);
    edges.convertTo();
    edges.grayscaleOtsu(clean);
    clean.negative(clean);
    result = &clean;
    break;

  case 1: // Gaussian gradients + magnitude.
    gray.gaussianGradients(gradX, gradY, 1.2f);
    mag.gradientMagnitude(gradX, gradY);
    mag.convertTo();
    mag.grayscaleOtsu(clean);
    result = &clean;
    break;

  case 2: // Sobel + magnitude.
    gray.filter2D(kernelX, gradX);
    gradX.convertTo();
    gray.filter2D(kernelY, gradY);
    gradY.convertTo();
    mag.gradientMagnitude(gradX, gradY);
    mag.convertTo();
    mag.grayscaleOtsu(clean);
    result = &clean;
    break;

  default:
    break;
  }

  if (result != nullptr) {
    serial.send(*result);
    serial.send1D(&current_mode, sizeof(int), 1, SERIAL_DATA_OTHER);
  }

  camera.capture(CONTINUOUS, previewRgb);
}

void setup() {
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  serial.init();
  camera.init(IMAGE_RES_QVGA, IMAGE_FORMAT_RGB565);

  previewRgb = embedDIP::Image(IMAGE_RES_QVGA, IMAGE_FORMAT_RGB565);
  gray = embedDIP::Image(IMAGE_RES_QVGA, IMAGE_FORMAT_GRAYSCALE);
  edges = embedDIP::Image(IMAGE_RES_QVGA, IMAGE_FORMAT_GRAYSCALE);
  clean = embedDIP::Image(IMAGE_RES_QVGA, IMAGE_FORMAT_GRAYSCALE);
  gradX = embedDIP::Image(IMAGE_RES_QVGA, IMAGE_FORMAT_GRAYSCALE);
  gradY = embedDIP::Image(IMAGE_RES_QVGA, IMAGE_FORMAT_GRAYSCALE);
  mag = embedDIP::Image(IMAGE_RES_QVGA, IMAGE_FORMAT_GRAYSCALE);
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
        current_mode = (current_mode + 1) % 3;
      }
    }
  }

  delay(5);
}
