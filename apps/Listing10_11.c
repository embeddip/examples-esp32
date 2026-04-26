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
Image *denoised = NULL;
Image *bin = NULL;
Image *morph = NULL;
Image *labels = NULL;

static int current_mode = 0;

static int lastBtn = HIGH;
static bool pressed = false;
static unsigned long tDown = 0;
static const unsigned long debounceMs = 40;
static const unsigned long longPressMs = 600;
static unsigned long lastChangeMs = 0;

static const float blur3x3[9] = {
    1.0f / 9.0f, 1.0f / 9.0f, 1.0f / 9.0f,
    1.0f / 9.0f, 1.0f / 9.0f, 1.0f / 9.0f,
    1.0f / 9.0f, 1.0f / 9.0f, 1.0f / 9.0f,
};

static void apply_current_mode() {
  camera->stop();

  cvtColor(previewRgb, gray, CVT_RGB565_TO_GRAYSCALE);
  filter2D(gray, denoised, blur3x3, 3);
  convertTo(denoised);

  Kernel k;
  getStructuringElement(&k, MORPH_ELLIPSE, 3);

  uint32_t numOfLabel = 0;

  switch (current_mode) {
  case 0: // Binary mask
    grayscaleThreshold(denoised, bin, 140);
    cvtColor(bin, morph, CVT_COPY);
    break;

  case 1: // Binary + opening
    grayscaleThreshold(denoised, bin, 140);
    opening(bin, morph, &k, 2);
    break;

  case 2: // Binary + closing
    grayscaleThreshold(denoised, bin, 140);
    closing(bin, morph, &k, 2);
    break;

  case 3: // Smart inspection
    grayscaleThreshold(denoised, bin, 140);
    opening(bin, morph, &k, 2);
    connectedComponents(morph, labels, &numOfLabel);
    normalize(labels);
    negative(labels, labels);
    cvtColor(labels, morph, CVT_COPY);
    break;

  default:
    break;
  }

  serial->send(morph);
  camera->capture(CONTINUOUS, previewRgb);
}

void setup() {
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  serial->init();
  camera->init(IMAGE_RES_QVGA, IMAGE_FORMAT_RGB565);

  createImage(IMAGE_RES_QVGA, IMAGE_FORMAT_RGB565, &previewRgb);
  createImage(IMAGE_RES_QVGA, IMAGE_FORMAT_GRAYSCALE, &gray);
  createImage(IMAGE_RES_QVGA, IMAGE_FORMAT_GRAYSCALE, &denoised);
  createImage(IMAGE_RES_QVGA, IMAGE_FORMAT_GRAYSCALE, &bin);
  createImage(IMAGE_RES_QVGA, IMAGE_FORMAT_GRAYSCALE, &morph);
  createImage(IMAGE_RES_QVGA, IMAGE_FORMAT_GRAYSCALE, &labels);
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
        current_mode = (current_mode + 1) % 4;
      }
    }
  }

  delay(5);
}
