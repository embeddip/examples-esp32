#include <Arduino.h>
#include <embedDIP.h>

#ifndef BUTTON_PIN
#define BUTTON_PIN 15
#endif

SET_LOOP_TASK_STACK_SIZE(16 * 1024 * 2);

serial_t *serial = &esp32_uart;
camera_t *camera = &esp32_ov2640;

Image *previewRgb = NULL;
Image *grayQvga = NULL;
Image *work128 = NULL;
Image *tmp128 = NULL;
Image *out128 = NULL;
Image *fftImg = NULL;
Image *filterImg = NULL;
Image *outQvga = NULL;

static int current_mode = 0;

static int lastBtn = HIGH;
static bool pressed = false;
static unsigned long tDown = 0;
static const unsigned long debounceMs = 40;
static const unsigned long longPressMs = 600;
static unsigned long lastChangeMs = 0;

static const float sharpenKernel[9] = {
    0, -1, 0,
    -1, 4, -1,
    0, -1, 0};

static void apply_current_mode() {
  camera->stop();

  cvtColor(previewRgb, grayQvga, CVT_RGB565_TO_GRAYSCALE);
  resize(grayQvga, work128, 128, 128);
  convertTo(work128);

  switch (current_mode) {
  case 0: // Spatial denoise.
    histEq(work128, tmp128);
    medianFilter(tmp128, out128, 3);
    convertTo(out128);
    break;

  case 1: // Spatial sharpen.
    filter2D(work128, tmp128, (const float *)sharpenKernel, 3);
    add(work128, tmp128, out128);
    convertTo(out128);
    break;

  case 2: // FFT Gaussian low-pass.
    fft(work128, fftImg);
    fftshift(fftImg);
    getFilter(filterImg, FREQ_FILTER_GAUSSIAN_LOWPASS, 30.0f, 0.0f);
    ffilter2D(fftImg, filterImg, out128);
    fftshift(out128);
    ifft(out128, out128);
    convertTo(out128);
    break;

  case 3: // FFT Gaussian high-pass.
    fft(work128, fftImg);
    fftshift(fftImg);
    getFilter(filterImg, FREQ_FILTER_GAUSSIAN_HIGHPASS, 25.0f, 0.0f);
    ffilter2D(fftImg, filterImg, out128);
    fftshift(out128);
    ifft(out128, out128);
    convertTo(out128);
    break;

  case 4: // Highboost: original - LPF(original).
    fft(work128, fftImg);
    fftshift(fftImg);
    getFilter(filterImg, FREQ_FILTER_GAUSSIAN_LOWPASS, 20.0f, 0.0f);
    ffilter2D(fftImg, filterImg, tmp128);
    fftshift(tmp128);
    ifft(tmp128, tmp128);
    difference(work128, tmp128, out128);
    convertTo(out128);
    break;

  default:
    break;
  }

  resize(out128, outQvga, outQvga->width, outQvga->height);
  convertTo(outQvga);
  serial->send(outQvga);

  camera->capture(CONTINUOUS, previewRgb);
}

void setup() {
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  serial->init();
  camera->init(IMAGE_RES_QVGA, IMAGE_FORMAT_RGB565);

  createImage(IMAGE_RES_QVGA, IMAGE_FORMAT_RGB565, &previewRgb);
  createImage(IMAGE_RES_QVGA, IMAGE_FORMAT_GRAYSCALE, &grayQvga);
  createImageWH(128, 128, IMAGE_FORMAT_GRAYSCALE, &work128);
  createImageWH(128, 128, IMAGE_FORMAT_GRAYSCALE, &tmp128);
  createImageWH(128, 128, IMAGE_FORMAT_GRAYSCALE, &out128);
  createImageWH(128, 128, IMAGE_FORMAT_GRAYSCALE, &fftImg);
  createImageWH(128, 128, IMAGE_FORMAT_GRAYSCALE, &filterImg);
  createImage(IMAGE_RES_QVGA, IMAGE_FORMAT_GRAYSCALE, &outQvga);
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
        current_mode = (current_mode + 1) % 5;
      }
    }
  }

  delay(5);
}
