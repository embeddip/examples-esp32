#include <Arduino.h>
#include <embedDIP.h>

#ifndef BUTTON_PIN
#define BUTTON_PIN 15
#endif

SET_LOOP_TASK_STACK_SIZE(16 * 1024 * 2);

serial_t *serial = &esp32_uart;
camera_t *camera = &esp32_ov2640;

Image *previewRgb = NULL;
Image *gray = NULL;
Image *edges = NULL;
Image *clean = NULL;
Image *gradX = NULL;
Image *gradY = NULL;
Image *mag = NULL;

static int current_mode = 0;

static int lastBtn = HIGH;
static bool pressed = false;
static unsigned long tDown = 0;
static const unsigned long debounceMs = 40;
static const unsigned long longPressMs = 600;
static unsigned long lastChangeMs = 0;

static const float kernelX[9] = {-1, 0, 1, -2, 0, 2, -1, 0, 1};
static const float kernelY[9] = {1, 2, 1, 0, 0, 0, -1, -2, -1};

static void apply_current_mode() {
  camera->stop();
  cvtColor(previewRgb, gray, CVT_RGB565_TO_GRAYSCALE);

  Image *result = NULL;

  switch (current_mode) {
  case 0: // LoG-based edge mask.
    logFilter(gray, edges, 1.4f);
    convertTo(edges);
    grayscaleOtsu(edges, clean);
    negative(clean, clean);
    result = clean;
    break;

  case 1: // Gaussian gradients + magnitude.
    gaussianGradients(gray, gradX, gradY, 1.2f);
    gradientMagnitude(gradX, gradY, mag);
    convertTo(mag);
    grayscaleOtsu(mag, clean);
    result = clean;
    break;

  case 2: // Sobel + magnitude.
    filter2D(gray, gradX, kernelX, 3);
    convertTo(gradX);
    filter2D(gray, gradY, kernelY, 3);
    convertTo(gradY);
    gradientMagnitude(gradX, gradY, mag);
    convertTo(mag);
    grayscaleOtsu(mag, clean);
    result = clean;
    break;

  default:
    break;
  }

  if (result != NULL) {
    serial->send(result);
    serial->send1D(&current_mode, sizeof(int), 1, SERIAL_DATA_OTHER);
  }

  camera->capture(CONTINUOUS, previewRgb);
}

void setup() {
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  serial->init();
  camera->init(IMAGE_RES_QVGA, IMAGE_FORMAT_RGB565);

  createImage(IMAGE_RES_QVGA, IMAGE_FORMAT_RGB565, &previewRgb);
  createImage(IMAGE_RES_QVGA, IMAGE_FORMAT_GRAYSCALE, &gray);
  createImage(IMAGE_RES_QVGA, IMAGE_FORMAT_GRAYSCALE, &edges);
  createImage(IMAGE_RES_QVGA, IMAGE_FORMAT_GRAYSCALE, &clean);
  createImage(IMAGE_RES_QVGA, IMAGE_FORMAT_GRAYSCALE, &gradX);
  createImage(IMAGE_RES_QVGA, IMAGE_FORMAT_GRAYSCALE, &gradY);
  createImage(IMAGE_RES_QVGA, IMAGE_FORMAT_GRAYSCALE, &mag);
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
        camera->capture(SINGLE, previewRgb);
        apply_current_mode();
      } else {
        current_mode = (current_mode + 1) % 3;
      }
    }
  }

  delay(5);
}
